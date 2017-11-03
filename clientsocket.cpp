#include "clientsocket.h"
#include <QApplication>

ClientSocket::ClientSocket(QObject *parent) :
    QObject(parent),
    begin(false)
{
    socket = new QTcpSocket(this);
}

void ClientSocket::ConnectSocketToHost(QString ip, int port)
{
    socket->connectToHost(ip, port);
}
void ClientSocket::DisconnectSocketFromHost()
{
   socket->disconnectFromHost();
}

 void ClientSocket::SendImage(cv::Mat img)
 {
     connect(socket,SIGNAL(readyRead()),this,SLOT(readData()));

     std::vector<uchar> data;
     imencode(".jpg",img,data);
     QString start ="666";


     QString len =QString("%1").arg((int)data.size(),8,10,QLatin1Char('0'));
     socket->write(start.toStdString().c_str(),start.size());
     socket->write(len.toStdString().c_str(),len.size());
//     qDebug()<<" start"<<start.size();
//     qDebug()<<"len "<<len.size();

     socket->write(reinterpret_cast<const char*>(data.data()),data.size());
 }

 void ClientSocket::readData()
 {
//    QByteArray data = socket->readAll();
//    qDebug() << data;
//     disconnect(socket,SIGNAL(readyRead()),this,SLOT(readData()));
     if(!begin)
     {
         dataArray += (socket->read(8- dataArray.length()));
         if(dataArray.length() < 8)
             return;
         begin = true;
          bytesToRecive= dataArray.toUInt();
         dataArray.clear();
     }
     if(begin)
     {
         dataArray += (socket->read(bytesToRecive- dataArray.length()));
         if(dataArray.length() < bytesToRecive)
             return;
         QString  str = QString::fromStdString(dataArray.toStdString());
         QStringList  strlist = str.split('!');
         for( int i=0; i<strlist.length()-1;i++)
         {
             QString label =strlist[i].section('_', 0, 0).trimmed();
             int Xmin = strlist[i].section('_', 1, 1).trimmed().toInt();
             int Ymin = strlist[i].section('_', 2, 2).trimmed().toInt();
             int Xmax = strlist[i].section('_', 3, 3).trimmed().toInt();
             int Ymax = strlist[i].section('_', 4, 4).trimmed().toInt();

             resVector.push_back( Result(label, Xmin,Ymin,Xmax,Ymax));

         //    qDebug()<<"label:"<<label<<" "<<"Xmin"<<Xmin<<" "<<"Ymin"<<Ymin<<" "<<"Xmax:"<<Xmax<<" "<<"Ymax:"<<Ymax;
             }
         emit sendresult(resVector );
         resVector.clear();

         begin =false;
         dataArray.clear();

     }

//disconnect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
//qDebug() << "once disconnected";
 }




