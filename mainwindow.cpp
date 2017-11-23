#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDate>
#include <QTime>
#include <opencv2/opencv.hpp>
#include<iostream>
#include <math.h>
#include <opencv2/imgproc.hpp>
#include<string>
#include<highgui.h>
#include<QString>
using namespace std;
using namespace cv;

//            Mat H = (cv::Mat_<double> (3,3)<< 63.80215288000583, -24.99346074781772, 204.699996948241,
//            -1.489585252823073, 25.85489268425039, 208.6999969482408,
//            -0.003954301047392333, -0.08002418316182458, 60);

QPoint correct(QPoint point) {
//    Mat H = (cv::Mat_<double> (3,3)<< 64.85824543232776, -24.97521992286458, 203.4999999999988,
//             -1.753895354647881, 25.53900947831834, 210.0000000000004,
//             -0.002843722459956857, -0.0801653765433768, 50);
            Mat H = (cv::Mat_<double> (3,3)<< 63.80215288000583, -24.99346074781772, 204.699996948241,
            -1.489585252823073, 25.85489268425039, 208.6999969482408,
            -0.003954301047392333, -0.08002418316182458, 60);

    H = H.inv();
 //   invert(H,H);
   QPoint res;
   res.setX((H.at<double>(0,0) * point.x() + H.at<double>(0,1) * point.y() + H.at<double>(0, 2))/(H.at<double>(2,0) * point.x() + H.at<double>(2,1) * point.y() + H.at<double>(2,2)));
   res.setY((H.at<double>(1,0) * point.x() + H.at<double>(1,1) * point.y() + H.at<double>(1, 2))/(H.at<double>(2,0) * point.x() + H.at<double>(2,1) * point.y() + H.at<double>(2,2)));
   return res;
}

//cv::cvPoint correct1(QPoint point) {
//    Mat H = (cv::Mat_<double> (3,3)<< 64.85824543232776, -24.97521992286458, 203.4999999999988,
//             -1.753895354647881, 25.53900947831834, 210.0000000000004,
//             -0.002843722459956857, -0.0801653765433768, 70);
//   cvPoint res;

//   res.x = ((H.at<double>(0,0) * point.x() + H.at<double>(0,1) * point.y() + H.at<double>(0, 2))/(H.at<double>(2,0) * point.x() + H.at<double>(2,1) * point.y() + H.at<double>(2,2)));
//   res.y = ((H.at<double>(1,0) * point.x() + H.at<double>(1,1) * point.y() + H.at<double>(1, 2))/(H.at<double>(2,0) * point.x() + H.at<double>(2,1) * point.y() + H.at<double>(2,2)));
//   return res;
//}

Pos operator - (Pos pos1, Pos pos2)
{
    pos1.x_pos -= pos2.x_pos;
    pos1.y_pos -= pos2.y_pos;
    pos1.theta -= pos2.theta;
    return pos1;
}

float  errorTransport( float error)
{
    if(error < -CV_PI)
    {
        error += 2*CV_PI;
    }
    else if( error > CV_PI)
    {
        error -= 2*CV_PI;
    }
    return error;
}

float angle(QPoint p1, QPoint p2);
MainWindow::MainWindow(QWidget *parent) ://构造函数初始化
    QMainWindow(parent),
    ui(new Ui::MainWindow)
  ,bytesToRecive(0)
  ,begin(false)
  ,tcpHasConnected(false)
  ,clientHasConnected(false) ,
  grid(480,vector<int>(640,0)),
  heuristic(480,vector<int>(640,0)),
//  cost{2,2,2,2,2.828,2.828,2.828,2.828},  
//  cost{2,2,2,2,4,4,4,4},
//  delta{{-2,0},{0,-2},{2,0},{0,2},{-2,-2},{2,-2},{2,2},{-2,2}}
    cost{2,2,2,2},
    delta{{-2,0},{0,-2},{2,0},{0,2}}

{
  // cost =1;
    cameraMatrix = (cv::Mat_<double>(3,3)<<602.1305028623834, 0, 305.425554714981,  0, 602.5595514999234, 229.9865955799518,
    0, 0, 1);
    distCoeffs = (cv::Mat_<double>(1,5)<<0.1831737947819169, -0.520313602279158, -0.005860976837188498, -0.002718407129263247, -0.7854837254841104);
    cout<<cameraMatrix<<endl;
    cout<<distCoeffs<<endl;
    ui->setupUi(this);
    clientsocket = new ClientSocket(this);
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(recivedata()));
    connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(disconnectFromHost()));
    ui->disconnect_pushButton->setEnabled(false);//初始化时disconnect false
    ui->disconnect_pushButton_2->setEnabled(false);

    ui->lineEdit->setText("192.168.199.168");
    ui->ComLineEdit->setValue(8889);

    ui->lineEdit_ip->setText("127.0.0.1");
    ui->lineEdit_port->setText("7777");

    connect(this,SIGNAL(HostAddress(QString,int)),clientsocket,SLOT(ConnectSocketToHost(QString,int)));
    connect(this,SIGNAL(DisConnectSignal()),clientsocket,SLOT(DisconnectSocketFromHost()));
    connect(this,SIGNAL(Transport(cv::Mat)),clientsocket,SLOT(SendImage(cv::Mat)));

    connect(clientsocket,SIGNAL(sendresult(QVector<Result>)),this,SLOT(plotResultImage(QVector<Result>)));

}
//通信协议为数据开头为666然后是包的长度，再然后包，都是JPEG格式的 640*480*3*8(8bit一个像素0~255)
void MainWindow::recivedata()//接收数据（图片）
{

    if(!begin)
    {
        bytesOfNum += tcpSocket->read(3 - bytesOfNum.length());//找开头666
        if(bytesOfNum.length() < 3)
            return;
        beginNum = bytesOfNum.toUInt();
        if(beginNum==666)
        {
            bytesOfNum.clear();//找到开头后清除之前读过的数据
            imageRecived.clear();
            bytesToRecive = 0;
            begin = true;//找到开头可以读数据了
        }
        else
        {
            bytesOfNum.remove(0,1);
        }
    }
    if(begin)
    {

        if(bytesToRecive == 0)
        {
            bytesOfNum += tcpSocket->read(10 - bytesOfNum.length());
            if(bytesOfNum.length() < 10)
                return;
            bytesToRecive =  bytesOfNum.toUInt();
            bytesOfNum.clear();
        }

        if( bytesToRecive != 0)
        {
            imageRecived += tcpSocket->read(bytesToRecive - imageRecived.length());
//            qDebug() << imageRecived.length() <<":" << bytesToRecive;
            if(imageRecived.length() >= bytesToRecive)
            {
                imageRecived = imageRecived.mid(0,bytesToRecive);

                std::vector<uchar> data(imageRecived.begin(),imageRecived.end());//转到标准命名空间

                cv::Mat img = cv::imdecode(data,CV_LOAD_IMAGE_COLOR);//将数据解码获得img
             imwrite("/home/xh/housePicture/10.jpg",img);

//              qDebug()<<"bytesToRecive";



           // undistort(img, imageclone,cameraMatrix, distCoeffs); //jj


//            cv::Mat result;
//            addWeighted(img,1,imageclone,-1,0,result);
//            imshow("result",result);

           imageclone = img.clone();
            emit Transport(imageclone);

 //      send_data_to_client("request");

                cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
                QImage q_img((const unsigned char *)img.data
                             ,img.cols
                             ,img.rows
                             ,img.cols*img.channels()
                             ,QImage::Format_RGB888);
                ui->label_3->setPixmap(QPixmap::fromImage(q_img));
                imageRecived.clear();
                begin = false;
            }
        }
    }
}

MainWindow::~MainWindow()//析构函数
{
    delete ui;
}


void MainWindow::disconnectFromHost()//和host没连接时
{
    qDebug()<<"Disconnected!";
    ui->disconnect_pushButton->setEnabled(false);//disconnect为false灰色，不可以按
    ui->connect_pushButton->setEnabled(true);//connect为true,可以按
}

void MainWindow::on_connect_pushButton_clicked()//当connect按下之后录视频   kl
{
    //Video 2
    videoWriter.open(QString("./" + QDate::currentDate().toString("yyyy_MM_dd")+QTime::currentTime().toString("_hh_mm_ss") +".avi").toStdString(), CV_FOURCC('M', 'J', 'P', 'G'), 20.0, cv::Size(640, 480));
    tcpSocket->connectToHost(ui->lineEdit->text(),ui->ComLineEdit->text().toInt());
    ui->disconnect_pushButton->setEnabled(true);//连接后，disconnect可以按true
    ui->connect_pushButton->setEnabled(false);//connect不能按
    tcpHasConnected = true;//tcp连接


   send_data_to_client("request");

//   setCarSpeed(0, 0, -1000, -4, 0, 0);

    //setCarSpeed(1,0,0);
}

void MainWindow::on_disconnect_pushButton_clicked()
{
    ui->disconnect_pushButton->setEnabled(false);
    ui->connect_pushButton->setEnabled(true);
    setCarSpeed(0,0,0, 0, 0,0);


    tcpSocket->disconnectFromHost();//host未连接

    //Video 3
    videoWriter.release();
    tcpHasConnected = false;//tcp 未连接

   }

void MainWindow::setCarSpeed(int x, int ex, int y, int ey, int z, int ez)
{
//    x = 1000;
//    ex = -4;
//    y = -1000;
//    ey = -4;
//    z = 0;
//    ez = 0;
//        x = -1000;
//        ex = -4;
//        y = 0;
//        ey = 0;
//        z = 0;
//        ez = 0;

    QString command = QString("s %1 %2 %3 %4 %5 %6 \n").arg(x).arg(ex).arg(y).arg(ey).arg(z).arg(ez);
//    qDebug()<<command;
    send_data_to_client(command);
}

void MainWindow::send_data_to_client(QString dat)  //kl
{
    if(tcpHasConnected)//树莓派连了
        tcpSocket->write(dat.toStdString().c_str());
}


void MainWindow::on_connect_pushButton_2_clicked()
{
    emit HostAddress(ui->lineEdit_ip->text(), ui->lineEdit_port->text().toInt());
   // qDebug()<<ui->lineEdit_ip->text()<<ui->lineEdit_port->text().toInt();
    ui->disconnect_pushButton_2->setEnabled(true);
    ui->connect_pushButton_2->setEnabled(false);
   send_data_to_client("request");

}


void MainWindow::on_disconnect_pushButton_2_clicked()
{
    emit DisConnectSignal();
    ui->disconnect_pushButton_2->setEnabled(false);
    ui->connect_pushButton_2->setEnabled(true);

}

void MainWindow::plotResultImage(QVector<Result> resVector)
{

//            Mat newimage = imageclone.clone();
//            undistort(imageclone, newimage,cameraMatrix, distCoeffs);
//            Mat  temp = newimage.clone();
             newimage = imageclone.clone();
            Mat H = (cv::Mat_<double> (3,3)<< 63.80215288000583, -24.99346074781772, 204.699996948241,
            -1.489585252823073, 25.85489268425039, 208.6999969482408,
            -0.003954301047392333, -0.08002418316182458, 60);



//            Mat H = (cv::Mat_<double> (3,3)<< 74.24013347791714, -17.13784161807934, 207.0000000000002,
//            0.2703027350401612, 43.23989486256691, 253.5,
//            0.001463647174071788, -0.04426301645424408, 60);

//            H = H.inv();
            warpPerspective(imageclone,newimage,H,imageclone.size(),WARP_INVERSE_MAP+INTER_LINEAR);
       //     cout<<"hello world"<<endl;



  //  qDebug()<<resVector.size();
    bool rec_flag = false, tri_flag = false, star_flag = false, heart_flag = false, diamond_flag = false, arrow_flag =false;

    QPoint centerOfRect, centerOfTri,centerOfStar, centerOfHeart,diamond_begin,diamond_end, arrow_begin,arrow_end;
    for(int i=0 ; i<resVector.size(); i++)
    {

        if( resVector[i].label == "rectangle")
        {
            cv::rectangle(imageclone,cvPoint(resVector[i].xMin,resVector[i].yMin),cvPoint(resVector[i].xMax,resVector[i].yMax),cvScalar(255,0,0),2);
            cv::putText(imageclone,"rectangle",cv::Point2d(resVector[i].xMin,resVector[i].yMin),   CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,0,0));
             rec_flag =true;
//             centerOfRect = (QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
             centerOfRect = correct(QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
        QPoint p1, p2;
           p1 = correct(QPoint(resVector[i].xMin, resVector[i].yMin));
           p2 = correct(QPoint(resVector[i].xMax, resVector[i].yMax));
           centerOfRect = QPoint((p1.x() + p2.x()) / 2 , (p1.y() + p2.y()) / 2);

//           p1 = (QPoint(resVector[i].xMin, resVector[i].yMin));
//           p2 = (QPoint(resVector[i].xMax, resVector[i].yMax));

            cv::rectangle(newimage,cvPoint(p1.x(),p1.y()),cvPoint(p2.x(),p2.y()),cvScalar(255,0,0),2);
        }

       else if( resVector[i].label == "triangle")
        {
            cv::rectangle(imageclone,cvPoint(resVector[i].xMin,resVector[i].yMin),cvPoint(resVector[i].xMax,resVector[i].yMax),cvScalar(0,0,255),2);
            cv::putText(imageclone,"triangle",cv::Point2d(resVector[i].xMin,resVector[i].yMin),   CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(0,0,255));
            tri_flag =true;

//            centerOfTri = (QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
            centerOfTri = correct(QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));

        QPoint p1, p2;
           p1 = correct(QPoint(resVector[i].xMin, resVector[i].yMin));
           p2 = correct(QPoint(resVector[i].xMax, resVector[i].yMax));
           centerOfTri = QPoint((p1.x() + p2.x()) / 2 , (p1.y() + p2.y()) / 2);

        }

       else if( resVector[i].label ==  "star")
        {
            cv::rectangle(imageclone,cvPoint(resVector[i].xMin,resVector[i].yMin),cvPoint(resVector[i].xMax,resVector[i].yMax),cvScalar(0,0,0),2);
            cv::putText(imageclone, "star",cv::Point2d(resVector[i].xMin,resVector[i].yMin),   CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(0,0,0));
             star_flag =true;
//             centerOfStar = (QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
             centerOfStar = correct(QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));

        QPoint p1, p2;
           p1 = correct(QPoint(resVector[i].xMin, resVector[i].yMin));
           p2 = correct(QPoint(resVector[i].xMax, resVector[i].yMax));
           centerOfStar = QPoint((p1.x() + p2.x()) / 2 , (p1.y() + p2.y()) / 2);

        }

        else if( resVector[i].label == "heart")
        {
            cv::rectangle(imageclone,cvPoint(resVector[i].xMin,resVector[i].yMin),cvPoint(resVector[i].xMax,resVector[i].yMax),cvScalar(0,255,0),2);
            cv::putText(imageclone,"heart",cv::Point2d(resVector[i].xMin,resVector[i].yMin),   CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(0,255,0));
            heart_flag =true;
//            centerOfHeart = (QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
            centerOfHeart = correct(QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));

        QPoint p1, p2;
           p1 = correct(QPoint(resVector[i].xMin, resVector[i].yMin));
           p2 = correct(QPoint(resVector[i].xMax, resVector[i].yMax));
           centerOfHeart = QPoint((p1.x() + p2.x()) / 2 , (p1.y() + p2.y()) / 2);

        }

        else if( resVector[i].label == "diamond")
        {
            cv::rectangle(imageclone,cvPoint(resVector[i].xMin,resVector[i].yMin),cvPoint(resVector[i].xMax,resVector[i].yMax),cvScalar(0,255,0),2);
            cv::putText(imageclone,"diamond",cv::Point2d(resVector[i].xMin,resVector[i].yMin),   CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(0,255,0));
            diamond_flag =true;
//            centerOfHeart = (QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
       //     centerOfHeart = correct(QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));

       // QPoint p1, p2;
           diamond_begin = correct(QPoint(resVector[i].xMin, resVector[i].yMin))-QPoint(30,30);
           diamond_end = correct(QPoint(resVector[i].xMax, resVector[i].yMax))+QPoint(30,30);
        //   centerOfHeart = QPoint((p1.x() + p2.x()) / 2 , (p1.y() + p2.y()) / 2);

        }

        else if( resVector[i].label == "arrow")
        {
            cv::rectangle(imageclone,cvPoint(resVector[i].xMin,resVector[i].yMin),cvPoint(resVector[i].xMax,resVector[i].yMax),cvScalar(0,255,0),2);
            cv::putText(imageclone,"arrow",cv::Point2d(resVector[i].xMin,resVector[i].yMin),  CV_FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(0,255,0));
            arrow_flag =true;
//            centerOfHeart = (QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));
  //          centerOfHeart = correct(QPoint( (resVector[i].xMin+resVector[i].xMax)/2, (resVector[i].yMin+resVector[i].yMax)/2));

      //  QPoint p1, p2;
           arrow_begin = correct(QPoint(resVector[i].xMin, resVector[i].yMin))-QPoint(30,30);
           arrow_end= correct(QPoint(resVector[i].xMax, resVector[i].yMax))+QPoint(30,30);
    //       centerOfHeart = QPoint((p1.x() + p2.x()) / 2 , (p1.y() + p2.y()) / 2);

        }


    }

 //   cv::rectangle(newimage,cvPoint(228,198),cvPoint(301,242),cvScalar(255,0,0),2);
 //   cv::rectangle(newimage,cvPoint(339,308),cvPoint(430,375),cvScalar(0,255,0),2);
//    cv::rectangle(newimage,cvPoint(500,430),cvPoint(578,476),cvScalar(0,0,255),2);

//            imshow("temp", newimage);
//            imwrite("/home/xh/housePicture/niaokan_45.jpg",newimage);
  static  int index =0;
static  float angleOfTarget =0;

 //if( (resVector.size() == 6) && rec_flag && tri_flag && star_flag && heart_flag&& diamond_flag && arrow_flag &&(index ==0 ))
 if( (resVector.size() >= 5) && rec_flag && tri_flag && star_flag && heart_flag&&  arrow_flag)
 //       if(1)
    {
        index  = index +1;
        float angleOfCar = angle(centerOfStar, centerOfHeart) -CV_PI;
         angleOfTarget = angle( centerOfTri ,centerOfRect) -CV_PI;
        QPoint centerOfCar = (centerOfHeart+ centerOfStar)/2;
        QPoint centerOfTarget =(centerOfRect + centerOfTri)/2;
        init = vector<int>({centerOfCar.x(),centerOfCar.y()});  //此处x,y的位置bu 应该互换
       goal = vector<int>({centerOfTarget.x(),centerOfTarget.y()}); //此处x,y的位置bu应该互换
      cv::rectangle(newimage,Rect(init[0],init[1],10,10),cvScalar(255,0,0),2);  //起始点像素坐标
         cv::rectangle(newimage,Rect(goal[0],goal[1],10,10),cvScalar(255,0,0),2);//目标点像素坐标

        //cv::rectangle(imageclone,cvPoint(339,308),cvPoint(430,375),cvScalar(0,255,0),2); //障碍物像素坐标 18+5+3
        //cv::rectangle(newimage,cvPoint(272,303),cvPoint(360,390),cvScalar(0,255,0),1);// 障碍物加车宽的像素坐标
        cv::rectangle(newimage,cvPoint(diamond_begin.x(),diamond_begin.y()),cvPoint(diamond_end.x(),diamond_end.y()),cvScalar(0,255,0),1);// 障碍物加车宽的像素坐标
        cv::rectangle(newimage,cvPoint(arrow_begin.x(),arrow_begin.y()),cvPoint(arrow_end.x(),arrow_end.y()),cvScalar(0,255,0),1);// 障碍物加车宽的像素坐标
      //  cv::rectangle(newimage,cvPoint(272,303),cvPoint(360,390),cvScalar(0,255,0),1);// 障碍物加车宽的像素坐标


//       init = vector<int>({248,289});
//      goal = vector<int>({327,396});

//       QPoint obs_start =  QPoint(298,329); //此处x,y的位置不互换

//       QPoint obs_end  = QPoint(334,364);  //此处x,y的位置不互换

//       create_grid({{obs_start,obs_end}});
   //  create_grid({{{298,329},{334,364}}});
        create_grid({ {{diamond_begin.x(),diamond_begin.y()},{diamond_end.x(),diamond_end.y()}} ,{{arrow_begin.x(),arrow_begin.y()},{arrow_end.x(),arrow_end.y()}} });  //将车的宽度考虑进去 18像素 鸟瞰图


     //  print_matrix(grid);
      create_heuristc(goal);
     //  print_matrix(heuristic);
       reverse(init.begin(), init.end());
       reverse(goal.begin(), goal.end());
//      init = vector<int>({289,248});
//     goal = vector<int>({396,327});

    // if(!( (arrow_begin.x()<=centerOfCar.x()<= arrow_end.x()) && (arrow_begin.y() <=centerOfCar.y()<=arrow_end.y())  ) )
     qDebug() << centerOfCar.x() << ", " << arrow_begin.x() << ", " << arrow_end.x();
      qDebug() << centerOfCar.y() << ", " << arrow_begin.y() << ", " << arrow_end.y();
       if( !((centerOfCar.x() >= arrow_begin.x()) && (centerOfCar.x() <=arrow_end.x()) && (centerOfCar.y() >= arrow_begin.y()) && (centerOfCar.y() <=arrow_end.y()) ))
//      if(((centerOfCar.x() < arrow_begin.x()) ||(centerOfCar.x() > arrow_end.x())) &&((centerOfCar.y() < arrow_begin.y()) ||(centerOfCar.y()> arrow_end.y())) )
    {
        cout<<"xhh"<<endl;
     search(grid,init,goal,cost);
     smooth(path,0.5,0.1,0.000001);
   }
       else
       {
           cout<<"error happend"<<endl;
       }

       show_path();
       image_path = newimage;
//       if(index == 0)
//       {
//           carControl({centerOfCar.x(), centerOfCar.y(), angleOfCar}, {centerOfTarget.x(), centerOfTarget.y(), angleOfTarget});
//           index = index +1;

//       }
      //index = index+1;

//       cv::rectangle(newimage,cvPoint(248,289),cvPoint(258,299),cvScalar(255,0,0),2);
//       cv::rectangle(newimage,cvPoint(298,329),cvPoint(334,364),cvScalar(0,255,0),2);
//       cv::rectangle(newimage,cvPoint(327,396),cvPoint(337,406),cvScalar(0,0,255),2);

                   imshow("temp", newimage);
                   imwrite("/home/xh/housePicture/niaokan_45.jpg",newimage);



//        cv::circle(imageclone,cvPoint(centerOfCar.rx(),centerOfCar.ry()),5,cvScalar(0,0,255),2);
//        cv::circle(imageclone,cvPoint(centerOfTarget.rx(),centerOfTarget.ry()),5,cvScalar(0,255,0),2);

// //          cv::circle(imageclone,cvPoint(centerOfCar.x(),centerOfCar.y()),5,cvScalar(0,0,255),2); hhh
//     //      cv::circle(imageclone,cvPoint(centerOfTarget.x(),centerOfTarget.y()),5,cvScalar(0,255,0),2); hh


    //    QPoint errorOfPos = centerOfCar - centerOfTarget; hh

   //     carControl( errorOfPos, angleOfCar);
//        qDebug() << angleOfCar;
//       centerOfTarget.x() = path[1][1];
//       centerOfTarget.y()=path[1][0]; // path的坐标系恰巧与像素坐标系相反
//          centerOfTarget.setX( path[15][1]);
//          centerOfTarget.setY( path[15][0]);
//        carControl({centerOfCar.x(), centerOfCar.y(), angleOfCar}, {centerOfTarget.x(),centerOfTarget.y(),angleOfTarget });
//        qDebug()<<angleOfCar;
//        qDebug()    <<centerOfCar;
      cout<<"start:  "<<centerOfCar.x()<<" "<<centerOfCar.y()<<centerOfTarget.x()<<centerOfTarget.y()<<endl;

    }
 //  else if( (resVector.size() == 6) && rec_flag && tri_flag && star_flag && heart_flag && diamond_flag && arrow_flag&&(index !=0 ))
  if( (resVector.size() >= 5) && rec_flag && tri_flag && star_flag && heart_flag && arrow_flag)
 {

      static int  j =15;
     QPoint centerOfCar = (centerOfHeart+ centerOfStar)/2;
     float angleOfCar = angle(centerOfStar, centerOfHeart) -CV_PI;
   //  qDebug()<<centerOfCar.x()<<centerOfCar.y()<<path[j][1]<<path[j][0]<<endl;

     cv::rectangle(image_path,cvPoint(centerOfCar.x(),centerOfCar.y()),cvPoint(centerOfCar.x()+1,centerOfCar.y()+1),cvScalar(0,0,255),1);
     //                image.at<Vec3b>(i,j)[0] = 0;
     //                image.at<Vec3b>(i,j)[1] = 0;
     //                image.at<Vec3b>(i,j)[2] = 0;
                             imshow("temp2", image_path);
                             imwrite("/home/xh/housePicture/niaokan_45.jpg",image_path);

//                        imshow("temp", newimage);
//                        imwrite("/home/xh/housePicture/niaokan_45.jpg",newimage);


     if(  abs( centerOfCar.x() - path[j][1]) <=5 && abs( centerOfCar.y() - path[j][0]) <=5 &&(j <path.size())  )
     {
          j= j +15;
          cout<<"hello j:"<<j<<endl;
        carControl({centerOfCar.x(), centerOfCar.y(), angleOfCar}, {path[j][1], path[j][0], angleOfTarget});

//          if(abs(centerOfCar.x() -goal[0]) <40 && abs(centerOfCar.y()-goal[1])<40 )
//          {
//         carControl({centerOfCar.x(), centerOfCar.y(), angleOfCar}, {path[j][1], path[j][0],angleOfTarget});
//          }
//          else
//          {
//           carControl({centerOfCar.x(), centerOfCar.y(), angleOfCar}, {path[j][1], path[j][0], - CV_PI/2});
//           cout<<angleOfTarget<<"*********************xh"<<endl;
//          }
     }
     else
     {
     carControl({centerOfCar.x(), centerOfCar.y(), angleOfCar}, {path[j][1], path[j][0], angleOfTarget});
   //  cout <<"hello xh"<<endl;
     }
   //  cout <<"j: "<<j<<endl;

 }
 else
    {
       cout<<"not found enough"<<endl;
            carControl({0,0,0}, {0,0,0});
//        carControl( QPoint(0,0), 0.0);

    }

//                   static int i=20;
//                   char path[100];
//                   sprintf(path, "/home/xh/xiaohuangrenbu0726/%04d.jpg", i++);
//   //                 cout << path << endl;
//                  cv::imwrite(path, imageclone);

    const uchar *pSrc = (const uchar*)imageclone.data;
    QImage qimg(pSrc, imageclone.cols,imageclone.rows, imageclone.step, QImage::Format_RGB888);
    QPixmap pix=QPixmap::fromImage(qimg.rgbSwapped());

ui->label_result->setPixmap(pix);
ui->label_result->setFixedHeight(qimg.height());
ui->label_result->setFixedWidth(qimg.width());

imageclone.empty();
send_data_to_client("request");
}



float angle(QPoint p1, QPoint p2)
{
    float angle_temp;
      float xx, yy;

      xx = p2.x()- p1.x();
      yy = p2.y() - p1.y();

      if (xx == 0.0)
          angle_temp = CV_PI / 2.0;
      else
          angle_temp = atan(fabs(yy / xx));

      if ((xx < 0.0) && (yy >= 0.0))
          angle_temp = CV_PI - angle_temp;
      else if ((xx < 0.0) && (yy < 0.0))
          angle_temp = CV_PI + angle_temp;
      else if ((xx >= 0.0) && (yy < 0.0))
          angle_temp = CV_PI * 2.0 - angle_temp;

      return (angle_temp);

}


vector<int> GetBaseAndExponent( float floatValue, int resolution = 4)
{
   int exponent = 0;
   int multipler= 0;
   int base = 0;

   if( floatValue == 0.0)
       return {0,0};
   else
   {
       exponent = int( 1.0 +log10(abs(floatValue)));
       multipler = int( pow(10, resolution-exponent));
       base = int( floatValue*multipler);
       return {base, exponent - resolution};
   }

}

void MainWindow::carControl(QPoint errorOfPosition, float  errorOfAngle)
{
    float errorOfX = errorOfPosition.x();
    float errorOfY = errorOfPosition.y();

    if(abs(errorOfAngle) <0.1)
    {
        errorOfAngle = 0;
        if(abs(errorOfX) < 15)
        {
            errorOfX =0;
        }
        else{
            if(errorOfX <0)
            {
                errorOfX =0.1;
            }
            else
            {
                errorOfX = -0.1;
            }
        }

        if(abs(errorOfY) < 15)
        {
            errorOfY =0;
        }
        else{
            if(errorOfY>0)
            {
                errorOfY =0.1;
            }
            else
            {
                errorOfY= -0.1;
            }
        }
    }
    else {
    if(errorOfAngle <0)
    {
        errorOfAngle = 0.2;
    }
    else
    {
        errorOfAngle = -0.2;
    }
    }
//    errorOfY = 0.0;
//    qDebug() <<errorOfPosition.y();
//    qDebug()<<errorOfY;
    vector<int> TargetOfAngle = GetBaseAndExponent(errorOfAngle);
    vector<int> TargetOfX = GetBaseAndExponent(errorOfX);
//    qDebug()<<TargetOfX[0];
//    qDebug()<<TargetOfX[1];
//    TargetOfX[0] =0;
//    TargetOfX[1] =0;
    vector<int> TargetOfY = GetBaseAndExponent(errorOfY);
    setCarSpeed(TargetOfX[0],TargetOfX[1],TargetOfY[0], TargetOfY[1], TargetOfAngle[0],TargetOfAngle[1]);
}


void MainWindow::carControl(Pos  cur, Pos tar){
    static int  num =0;
    Pos error = cur - tar;
    float errorOfTheta =0;
    float errorOfX =0;
    float errorOfY =0;
//    qDebug() << cur.theta - tar.theta << "  ,  "  << error.theta;
   error.theta = errorTransport( error.theta);   //确保旋转角度最小
//   qDebug() << cur.theta << "  ,   " << tar.theta << " , "<< error.theta;
   float sinThe = sin(cur.theta);
   float cosThe = cos(cur.theta);
   cv::Mat R(2,2,CV_32F,Scalar::all(0));
   R.at<float>(0,0) = cosThe; R.at<float>(0, 1) = -sinThe;
   R.at<float>(1,0) = sinThe; R.at<float>(1, 1) = cosThe;
//   qDebug() << "before" ;
//   qDebug() << R.at<float>(0,0)  << " " << R.at<float>(0,1);
//   qDebug() << R.at<float>(1,0)  << " " << R.at<float>(1,1);
    cv::Mat R_inv = R.inv();
//   qDebug() << "after" ;
//   qDebug() << R_inv.at<float>(0,0)  << " " << R_inv.at<float>(0,1);
//   qDebug() << R_inv.at<float>(1,0)  << " " << R_inv.at<float>(1,1);

    int errorOfXPos =  R_inv.at<float>(0,0) *error.x_pos +  R_inv.at<float>(0,1) *error.y_pos; //将目标点的坐标投影到小车坐标系上
    int errorOfYPos =  R_inv.at<float>(1,0) *error.x_pos +  R_inv.at<float>(1,1) *error.y_pos;
    qDebug()<<"num:"<<num<<"errorOfXPos_Tran: "<<errorOfXPos <<"errorOfYPos_Tran: "<<errorOfYPos;
    if(abs(error.theta) < 0.1)
    {
        if( abs(errorOfXPos) >=5)
        {
            errorOfX = errorOfXPos < 0 ? 0.1 : -0.1;
        }
        if( abs(errorOfYPos) >=5)

        {
           errorOfY = errorOfYPos > 0 ? 0.1 : -0.1;
           //cout<<"errorOfY"<<errorOfY<<endl;
        }

    }
    else
    {
        errorOfTheta = error.theta < 0 ? 0.1: -0.1;
    }

    vector<int> TargetOfX = GetBaseAndExponent((errorOfX));
    vector<int> TargetOfY = GetBaseAndExponent((errorOfY));
    vector<int> TargetOfTheta = GetBaseAndExponent(errorOfTheta);
   setCarSpeed(TargetOfX[0],TargetOfX[1],TargetOfY[0], TargetOfY[1], TargetOfTheta[0],TargetOfTheta[1]);
   num = num +1;
}

void MainWindow::search(vector<vector<int> >grid, vector<int> init, vector<int> goal, vector<int> cost)
{
    vector<vector<int>> closed(grid.size(),vector<int>(grid[0].size(),0));
    vector<vector<int>> expand(grid.size(),vector<int>(grid[0].size(),-1));
    vector<vector<char>>  result(grid.size(),vector<char>(grid[0].size(),' 0'));
    QVector<QVector<int>> action(grid.size(),QVector<int>(grid[0].size(),-1));
//    for(int i=0; i<grid.size(); i++)
//    {
//       // cout<<i<<endl;
//        for(int j=0; j<grid[0].size(); j++)
//        {
//            //cout<<j<<endl;
//            closed[i].push_back(0);
//         }
//    }
    closed[init[0]][init[1]] =1;
     int x = init[0];
     int y = init[1];
     int g =0;
     int h = heuristic[x][y];
     int f = g +h;
     int count =0;
     vector<vector<int>> open{{f,g,h,x,y}};

     bool found = false;  //flag that is set when search is complete
     bool resign = false;  // flag set if we can't find expand
     while(found == false && resign== false)
     {
         if(open.size() == 0)
         {
             resign =true;
             path.clear();
             cout<<"fail"<<endl;
         }
         else
         {
             //如何删除最小g值元素的代码
           //  sort(open.begin(), open.end(), greater<vector<int>());
             sort(open.begin(),open.end());
         //    reverse(open.begin(),open.end());
             vector<int> next = open[0];
             open.erase(open.begin());
             int x = next[3];
             int y = next[4];
             int h = next[2];
             int g = next[1];
             int f = next[0];
             expand[x][y] = count;
             count +=1;
            cout<<"count:"<<count<<endl;
           // cout<<x<<" "<<y<<" "<<g<<endl;

             //判断是否是目标点
             if((abs(x- goal[0]) <3) && (abs(y -goal[1])) <3)
//             if( x== goal[0] && y== goal[1])
             {
                 found = true;
                //cout<< f<<" "<<g<<" "<<h<<" "<<x<<" "<<y<<endl;
                QPoint u{x,y};
                result[u.x()][u.y()]='*';
                path.clear();
                path.push_back({u.x(),u.y()});
                 int j=0;
                while( u.x() !=init[0] || u.y()!= init[1])
                {
                    int i = action[u.x()][u.y()];
                    //cout<<i<<endl;
              //      cout<<"hello world  "<<"ux: "<<u.x()<<"uy: "<<u.y()<<endl;

                    u  = u - QPoint(delta[i][0],delta[i][1]) ;
                 //   cout<<u.x()<<" "<<u.y()<<endl;
                      vector<int> point_location{u.x(), u.y()};

                    path.push_back(point_location);
                  //  cout<<"Xh"<<endl;


                   result[u.x()][u.y()]= delta_name[i];
               //    cout<<"count"<<j<<endl;
              //     j =j+1;


                }
               reverse(path.begin(), path.end());
       //        print_matrix(path);
//                   for(int i=0; i<result.size(); i++)
//                   {
//                       for(int j=0; j<result[0].size(); j++)
//                       {
//                          cout<<result[i][j]<<" ";
//                        }
//                       cout<<endl;
//                   }

        //    print_matrix(result);

        //        print_matrix(expand);
//                                image.at<Vec3b>(u.x(),u.y())[0]=0;
//                                image.at<Vec3b>(u.x(),u.y())[1]=0;
//                                image.at<Vec3b>(u.x(),u.y())[2]=255;

//                                while( u.x() !=init[0] || u.y()!= init[1])
//                                {
//                                //    u =path[u.x()][u.y()];
//                                    u  = u + QPoint(delta[action[u.x()][u.y()]][0],delta[action[u.x()][u.y()]][1]);
//                                  image.at<Vec3b>(u.x(),u.y())[0]=0;
//                                  image.at<Vec3b>(u.x(),u.y())[1]=0;
//                                  image.at<Vec3b>(u.x(),u.y())[2]=255;
//                                }
//                                 image.at<Vec3b>(init[0],init[1])[0] = 0;
//                                 image.at<Vec3b>(init[0],init[1])[1] = 0;
//                                 image.at<Vec3b>(init[0],init[1])[2] = 255;

//                                 cv::imshow("b",image);
//                                 cv::waitKey(0);
          //      print_matrix(result);

             }
             //核心程序，扩展
             else
             {
                 for(int i=0; i<delta.size();i++)
                 {
                     int x2 = x+delta[i][0];
                     int y2 = y+delta[i][1];
                    //  static int j=0;
                      if( x2 >=0 && x2<grid.size() && y2>=0 && y2 <grid[0].size() && grid[x2][y2]==0 && closed[x2][y2]==0)
                     {
                       int g2  = g+cost[i];
                       int h2 = heuristic[x2][y2];
                       int f2 = g2 + h2;
                       open.push_back({f2,g2,h2,x2,y2});
                       closed[x2][y2] =1;
                       action[x2][y2] = i;
                    //   j=j+1;
              //         cout<<j<<" : "<<x2<<" "<<y2<<" "<<grid[x2][y2]<<endl;


                     }
                 }
             }

         }


     }
//     for(int i=0; i<expand.size();i++)
//     {
//         for(int j=0; j<expand[0].size(); j++)
//         {
//             cout<<expand[i][j]<<" ";
//         }
//         cout<<endl;
//     }


}
void MainWindow::create_grid(vector<vector<QPoint> > obs)
{
//    vector<vector<int> > res;
//    int delta_x = goal.x() - start.x();
//    int delta_y = goal.y() - start.y();
//    res.assign(delta_x, vector<int>(delta_y,0));
    for(int i=0; i<obs.size(); i++)
    {
        QPoint Point_start= obs[i][0] ;
        QPoint Point_end = obs[i][1] ;
        int delta_x1 = Point_end.x() - Point_start.x()+1;
        int delta_y1 = Point_end.y() - Point_start.y()+1;
        vector<int> u(delta_x1,1);
        for(int j=0; j<delta_y1; j++)
        {
            copy(u.begin(),u.end(),grid[Point_start.y()+j].begin()+Point_start.x());
        }

    }
//    return res;
}

  void MainWindow::create_heuristc(vector<int> goal)
  {


      for(int i=0; i<grid.size();i++)
      {
          for(int j=0; j<grid[0].size(); j++)
          {
              heuristic[i][j] = abs(j-goal[0])+ abs(i-goal[1]);
          }
      }

  }
  void MainWindow::show_path()
  {
      path = newpath;
      for(int i=0; i<path.size();i++)
      {

      cv::rectangle(newimage,cvPoint(path[i][1],path[i][0]),cvPoint(path[i][1]+1,path[i][0]+1),cvScalar(0,255,0),1);
      }
     //   imshow("result",imageclone);
     //   imwrite("/home/xh/1.jpg",imageclone);
     // waitKey(0);
  }

  template<class T>
  void MainWindow::print_matrix(vector<vector<T> > matrix)
  {
      for(int i=0; i<matrix.size();i++)
      {
          for(int j=0; j<matrix[0].size(); j++)
          {
              cout<<matrix[i][j]<<" ";
          }
          cout<<endl;
      }

  }

  void MainWindow::smooth(vector<vector<int> > path, float weight_data, float weight_smooth, float tolerance)
  {
      newpath = path;
      float change= tolerance;
    //  print_matrix(newpath);
     while( change >= tolerance )
     {
         change =0;
      for(int i=1; i<path.size()-1; i++)
      {
          for(int j=0; j<path[0].size(); j++)
          {
              float aux = newpath[i][j];
              newpath[i][j] = newpath[i][j]+0.5*(path[i][j]-newpath[i][j]);
              newpath[i][j] =  newpath[i][j] +0.1*( newpath[i+1][j] +newpath[i-1][j]-2* newpath[i][j] );
              change = change + abs(aux - newpath[i][j]);
          }
    }
     // cout<<"change:"<<change<<endl;

      }
  }
