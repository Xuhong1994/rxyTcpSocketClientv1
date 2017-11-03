#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <QObject>
#include <QTcpSocket>
#include <opencv2/opencv.hpp>
#include<QVector>

struct Result{
    QString label;
    int xMin;
    int yMin;
    int xMax;
    int yMax;
 Result()
 {

 }
 Result(QString la, int xmin, int ymin, int xmax, int ymax ):
     label(la),
     xMin(xmin),
     yMin(ymin),
     xMax(xmax),
     yMax(ymax)
 {

 }
};

using namespace cv;
class ClientSocket : public QObject
{
    Q_OBJECT
public:
    explicit ClientSocket(QObject *parent = 0);

signals:
  void   sendresult(QVector<Result>);

public slots:
   void ConnectSocketToHost(QString, int);
   void DisconnectSocketFromHost();
   void SendImage(cv::Mat img);
   void readData();



private:
    QTcpSocket  *socket;
//    QByteArray  getdata(int length);
    QByteArray dataArray;
    bool begin;
    quint32 bytesToRecive;
    QVector<Result>  resVector;




};

#endif // CLIENTSOCKET_H
