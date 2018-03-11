#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        if(serial.open(QIODevice::ReadWrite))
        {
            ui->PortBox->addItem(serial.portName());
            serial.close();
        }
    }

    ui->BaudBox->setCurrentIndex(7);
    ui->BitNumBox->setCurrentIndex(3);
    setupRealtimeDataDemo(ui->qwtPlot);

    //用于间隔发送的定时器
    connect(&txTimer_request,SIGNAL(timeout()),this,SLOT(Request_Data()));
    connect(&txTimer_defpar,SIGNAL(timeout()),this,SLOT(Def_Par()));
//    connect(&updateTimer,SIGNAL(timeout()),this,SLOT(updatedataSlot()));
//    updateTimer.start(500);

     slave_address.append(4,0xFF); //默认从机地址
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupRealtimeDataDemo(QwtPlot *qwtplot)
{
    for(int i=0;i<110;i++){
        xdata.append(i);
        ydata.append(0);
    }
    demoName="实时加速度数据";
    qwtplot->setTitle(demoName);
    qwtplot->setCanvasBackground(Qt::white);

    curve= new QwtPlotCurve();
    curve->setTitle("振动加速度");
    curve->setPen(Qt::blue,1);

    qwtplot->setAxisScale(QwtPlot::yLeft,-50,50,0);
    qwtplot->setAxisTitle(QwtPlot::yLeft,"m^2/s");
    qwtplot->setAxisScale(QwtPlot::xBottom,0,120,0);

    QwtPlotGrid *grid=new QwtPlotGrid();
    grid->enableX(true);
    grid->enableY(true);
    grid->setMajorPen(Qt::black,0,Qt::DotLine);
    grid->attach(qwtplot);

}


void MainWindow::updatedataSlot()
{
    curve->setSamples(xdata,ydata);
    curve->attach(ui->qwtPlot);
    ui->qwtPlot->replot();
}

//处理接收到的串口数据
void MainWindow::Read_Data()
{
    QByteArray rxbuf;
    QTime curtime;
    uchar data1,data2;
    uint data=0;
    QString sense_id="";
    double acceleration=0;
    double temper_data=0;
    double power_data=0;

     //接收采集数据
    if(ui->pushButton_receive->text()==tr("停止接收"))
    {
        if(serial->bytesAvailable()<228) //接收到完整一帧，228字节
        {
            return;
        }
        else
        {
            //读取一帧数据
            rxbuf = serial->read(228);
            //读取从机地址
            for(int i= 1;i<5;i++)
            {
                sense_id.append(QString::number(rxbuf[i]&0xff,16));
                sense_id.append(" ");
            }
            ui->label_sense_id_value->setText(sense_id);
            //记录接收时间
            curtime=curtime.currentTime();
            ui->label_receive_time_value->setText(curtime.toString());
            //删除帧头、从机地址
            rxbuf.remove(0,5);
            //处理数据部分
            for (int i = 1; i < rxbuf.length(); i=i+2)
            {
                 data1=rxbuf.at(i);
                 data2=rxbuf.at(i+1);
                 data = (data1<<8)+data2;
                    if(i<=2)
                    {
                       temper_data=((double)data/4095*2.2*1.5-0.5)/0.01;
                       ui->label_temper_value->setText(QString("%1").arg(temper_data,0,'f',2));
//                     ui->label_temper_value->setText(QString("%1").arg((uint)data,2,16,QLatin1Char(' ')));
                    }
                    else if(i<=4)
                    {
                        power_data=(double)data/4095*2.2*1.5;
                        ui->label_power_value->setText(QString("%1").arg(power_data,0,'f',2));
//                      ui->label_power_value->setText(QString("%1").arg((uint)data,2,16,QLatin1Char(' ')));
                    }
                    else{
//                    xdata.removeFirst();
//                    xdata.append(curtime.msecsSinceStartOfDay()+1);
                    ydata.removeFirst();
                    acceleration=(((double)data-2048)/4095*2.2*1.5*1000)/40;
                    ydata.append(acceleration);
                    }
            }
        updatedataSlot();
        }
      }
    else //修改系统参数
        {
        if(serial->bytesAvailable()>=6)
        {
            //读取一帧数据
            rxbuf = serial->read(6);
            //读取从机地址
            for(int i= 1;i<5;i++)
            {
                sense_id.append(QString::number(rxbuf[i]&0xff,16));
                sense_id.append(" ");
            }

            if(rxbuf.at(5)==0x06)
            ui->statusBar->showMessage((sense_id+":传感器参数修改完成"),5000);
            if(rxbuf.at(5)==0x16)
            ui->statusBar->showMessage((sense_id+":传感器参数修改未完成"),5000);

       }
    }
}

//向从机索取数据
void MainWindow::Request_Data()
{
    QByteArray txbuf;
    txbuf.append(0xAA);
    txbuf.append(slave_address);
    txbuf.append(0x0B);
    serial->write(txbuf);

}
//修改传感器参数
void MainWindow::Def_Par()
{
    static int count=0;
    QByteArray txbuf;
    txbuf.append(0xAA);
    txbuf.append(slave_address);
    txbuf.append(0x0A);
    txbuf.append(sleeptime);
    txbuf.append(wakeuptime);
    txbuf.append(sample_freq);
    serial->write(txbuf);
    if(count>9)
    {
        txTimer_defpar.stop();
        count=0;
     }
    else
        count++;
 }

void MainWindow::on_openButton_clicked()
{
    if(ui->openButton->text()==tr("打开串口"))
    {
        serial = new QSerialPort;
        //设置串口名
        serial->setPortName(ui->PortBox->currentText());
        //打开串口
        serial->open(QIODevice::ReadWrite);
        //设置波特率
//        serial->setBaudRate(ui->BaudBox->currentText().toInt());
        switch(ui->BaudBox->currentIndex())
        {
        case 0:
                serial->setBaudRate(QSerialPort::Baud1200,QSerialPort::AllDirections);
                break;
            case 1:
                serial->setBaudRate(QSerialPort::Baud2400,QSerialPort::AllDirections);
                break;
            case 2:
                serial->setBaudRate(QSerialPort::Baud4800,QSerialPort::AllDirections);
                break;
            case 3:
                serial->setBaudRate(QSerialPort::Baud9600,QSerialPort::AllDirections);
                break;
            case 4:
                serial->setBaudRate(QSerialPort::Baud19200,QSerialPort::AllDirections);
                break;
            case 5:
                serial->setBaudRate(QSerialPort::Baud38400,QSerialPort::AllDirections);
                break;
            case 6:
                serial->setBaudRate(QSerialPort::Baud57600,QSerialPort::AllDirections);
                break;
            case 7:
                serial->setBaudRate(QSerialPort::Baud115200,QSerialPort::AllDirections);
                break;
            default:  break;
        }
        //设置数据位数
        switch(ui->BitNumBox->currentIndex())
        {
        case 0: serial->setDataBits(QSerialPort::Data5); break;
        case 1: serial->setDataBits(QSerialPort::Data6); break;
        case 2: serial->setDataBits(QSerialPort::Data7); break;
        case 3: serial->setDataBits(QSerialPort::Data8); break;
        default: break;
        }
        //设置奇偶校验
        switch(ui->ParityBox->currentIndex())
        {
        case 0:
            serial->setParity(QSerialPort::NoParity);
            break;
        case 1:
            serial->setParity(QSerialPort::OddParity);
            break;
        case 2:
            serial->setParity(QSerialPort::EvenParity);
            break;
        default:  break;
        }
        //设置停止位
        switch(ui->StopBox->currentIndex())
        {
        case 0: serial->setStopBits(QSerialPort::OneStop); break;
        case 1: serial->setStopBits(QSerialPort::TwoStop); break;
        default: break;
        }
        //设置流控制
        serial->setFlowControl(QSerialPort::NoFlowControl);
        //关闭设置菜单使能
        ui->PortBox->setEnabled(false);
        ui->BaudBox->setEnabled(false);
        ui->BitNumBox->setEnabled(false);
        ui->ParityBox->setEnabled(false);
        ui->StopBox->setEnabled(false);
        ui->pushButton_receive->setEnabled(true);
        ui->pushButton_def_par->setEnabled(true);
        ui->pushButton_startupdate->setEnabled(true);
        ui->openButton->setText(tr("关闭串口"));
        //连接信号槽
        QObject::connect(serial, &QSerialPort::readyRead, this, &MainWindow::Read_Data);
        ui->statusBar->showMessage("串口打开成功",3000);
    }
    else
    {
        //关闭串口
        serial->clear();
        serial->close();
        serial->deleteLater();
        //恢复设置使能
        ui->PortBox->setEnabled(true);
        ui->BaudBox->setEnabled(true);
        ui->BitNumBox->setEnabled(true);
        ui->ParityBox->setEnabled(true);
        ui->StopBox->setEnabled(true);
        ui->pushButton_receive->setEnabled(false);
        ui->pushButton_def_par->setEnabled(false);
        ui->pushButton_startupdate->setEnabled(false);
        ui->openButton->setText(tr("打开串口"));
        ui->statusBar->showMessage("串口已关闭",3000);
    }
}


void MainWindow::on_pushButton_receive_clicked()
{
    if(ui->pushButton_receive->text()==tr("开始接收"))
    {
       ui->pushButton_receive->setText(tr("停止接收"));
       ui->statusBar->showMessage("开始接收采集数据",3000);
       ui->pushButton_def_par->setEnabled(false);
       ui->pushButton_startupdate->setEnabled(false);
       ui->openButton->setEnabled(false);
       txTimer_request.start(1000);
    }
    else
    {
        ui->pushButton_receive->setText(tr("开始接收"));
        ui->statusBar->showMessage("停止接收采集数据",3000);
        ui->pushButton_def_par->setEnabled(true);
        ui->pushButton_startupdate->setEnabled(true);
        ui->openButton->setEnabled(true);
        txTimer_request.stop();
    }
    serial->clear();
}


void MainWindow::on_pushButton_def_par_clicked()
{
    sleeptime.clear();
    wakeuptime.clear();
    sample_freq.clear();
    sleeptime.append(ui->lineEdit_sleeptime->text().toUInt());
    wakeuptime.append(ui->lineEdit_wakeuptime->text().toUInt());
    sample_freq.append(ui->lineEdit_sample_freq->text().toUInt());
    txTimer_defpar.start(1000);
}


void MainWindow::on_pushButton_startupdate_clicked()
{
    qint8 frame_num;
    QByteArray file_data;
    filename=ui->lineEdit_IAPfile->text();
    QFileInfo fileinfo(filename);
    qint64 filesize=fileinfo.size();
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QByteArray up_ver,file_buf;
    file_buf=file.readAll();
    up_ver.append(0x11);
    Send_Ver(up_ver);
    frame_num=filesize/125;
    for(qint8 i=0;i<=frame_num;i++)
    {
    connect(&updateTimer,SIGNAL(timeout()),this,SLOT(Send_Package(file_buf)));
    updateTimer.start(4000);
    }
}

void MainWindow::on_pushButton_sel_IAPfile_clicked()
{
    filename = QFileDialog::getOpenFileName(this,
        tr("选择文件"),
        "",
        tr("All Files(*.*);;Binary Files (*.bin)")); //选择路径
    if(filename!="")
        ui->lineEdit_IAPfile->setText(filename);
}


void MainWindow::Send_Ver(const QByteArray &up_ver)
{
    QByteArray txbuf;
    QByteArray slave_address;

    slave_address.append(4,0xFF);

    txbuf.append(0xAA);
    txbuf.append(slave_address);
    txbuf.append(0x0F);
    txbuf.append(up_ver);
    serial->write(txbuf);
    txbuf.clear();
}
void MainWindow::Send_Package(QByteArray &file_buf)
{
    QByteArray txbuf;
    const char* file;
    file=file_buf.data();
    txbuf.append(0xDD);
    txbuf.append(file,125);
    serial->write(txbuf);
    txbuf.clear();
    file_buf.remove(0,125);

}

