#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <math.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <QtSerialPort/QtSerialPort>
#include <qwt_date_scale_draw.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setupRealtimeDataDemo(QwtPlot *qwtplot);

private:
    Ui::MainWindow *ui;
    QVector<double> xdata;
    QVector<double> ydata;
    QTimer updateTimer;
    QTimer txTimer_request;
    QTimer txTimer_defpar;
    QString demoName;
    QwtPlotCurve *curve ;
    QSerialPort *serial;
    QString filename;

    QByteArray slave_address;
    QByteArray sleeptime;
    QByteArray wakeuptime;
    QByteArray sample_freq;


private slots:
    void updatedataSlot();
    void on_openButton_clicked();
    void Request_Data();
    void Def_Par();
    void Send_Ver(const QByteArray &up_ver);
    void Send_Package(QByteArray &file_buf);
    void Read_Data();
    void on_pushButton_receive_clicked();
    void on_pushButton_startupdate_clicked();
    void on_pushButton_sel_IAPfile_clicked();
    void on_pushButton_def_par_clicked();
};

#endif // MAINWINDOW_H
