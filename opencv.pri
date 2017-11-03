win32{
	#����OpenCV����ֻ���������У�release��debug�Զ�����
        OpenCV_Dir = f:\opencv\build
        OpenCV_VerNum = 249 #������opencv_calib3d2413�е����֣��Լ��ĳ��Լ��İ汾����
	OpenCV_Plantformat = "x64" #d:\opencv\build\x64 ,�ĳ��Լ���plangformat
        OpenCV_VS_Version = vc12 #d:\opencv\build\x64\vc12 ���Լ���VC��Ӧ�İ汾���Լ��ĳɶ�Ӧ

	OpenCV_Libs =opencv_calib3d \
					opencv_contrib \
					opencv_core \
					opencv_features2d \
					opencv_flann \
					opencv_gpu \
					opencv_highgui \
					opencv_imgproc \
					opencv_legacy \
					opencv_ml \
					opencv_nonfree \
					opencv_objdetect \
					opencv_ocl \
					opencv_photo \
					opencv_stitching \
					opencv_superres \
					opencv_ts \
					opencv_video \
					opencv_videostab

	INCLUDEPATH += $${OpenCV_Dir}\include

	OpcnCV_LIBS_Dir = $${OpenCV_Dir}\\$${OpenCV_Plantformat}\\$${OpenCV_VS_Version}\lib

	CONFIG(debug,debug|release){
		message("debug build")
		for(lib,OpenCV_Libs){
			lib_full_path = $${OpcnCV_LIBS_Dir}\\$${lib}$${OpenCV_VerNum}d
			LIBS += -l$${lib_full_path}
		}
	}


	CONFIG(release,debug|release){
	message("release build")
		for(lib,OpenCV_Libs){
			lib_full_path = $${OpcnCV_LIBS_Dir}\\$${lib}$${OpenCV_VerNum}
			LIBS += -l$${lib_full_path}
		}
	}
}
