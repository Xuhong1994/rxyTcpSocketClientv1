#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <opencv2/opencv.hpp>
#include "clientsocket.h"
namespace Ui {
class MainWindow;
}

struct Pos{
    int x_pos;
    int y_pos;
    double theta;
    Pos() = default;
    Pos(int x, int y, double th):
        x_pos(x),
        y_pos(y),
        theta(th){
    }
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void recivedata();//收数据
    void disconnectFromHost();//不连接host(ip)

    void on_connect_pushButton_clicked();//connect按钮

    void on_disconnect_pushButton_clicked();//disconnect按钮

    void send_data_to_client(QString dat);//给client发数据

    void setCarSpeed(int x,int i, int y,int j, int z, int k);//设定车的速度

    void on_connect_pushButton_2_clicked();

    void on_disconnect_pushButton_2_clicked();
    void plotResultImage(QVector<Result>);

private:
    bool tcpHasConnected;
    cv::VideoWriter videoWriter;
    Ui::MainWindow *ui;
    QTcpSocket *tcpSocket;
    bool begin;
    quint32 beginNum;
    quint32 bytesToRecive;
    QByteArray imageRecived;
    QByteArray bytesOfNum;

   ClientSocket *clientsocket;

     cv::Mat imageclone ;

   bool clientHasConnected; ///////hxb
   void carControl(QPoint , float );
   void carControl(Pos , Pos );

   Mat cameraMatrix;
   Mat distCoeffs;



signals:
    void HostAddress(QString,int);
    void DisConnectSignal();
    void Transport(cv::Mat);

private:
    vector< vector<int> >  grid;
    vector< vector<int> >  heuristic;
    vector<int> init;
    int cost;
    vector<int> goal;
    vector<vector<int>> delta;
   vector<char> delta_name{'^','<','V','>'};
    void search(vector<vector<int> > grid, vector<int> init, vector<int> goal, int cost);
    void create_grid(vector<vector<QPoint>> );
    void create_heuristc(vector<int> goal);

    vector<vector<int>> path;
    template<class T>
    void print_matrix(vector<vector<T> > matrix);
    void show_path();
    Mat newimage;
    Mat image_path;
    void smooth(vector<vector<int> > path, float weight_data, float  weight_smooth, float tolerance );
    vector<vector<int>> newpath;



};

#endif // MAINWINDOW_H
