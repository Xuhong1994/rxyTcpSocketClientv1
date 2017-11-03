from v4l2 import *
import fcntl
import mmap
import select
import time
import Image
import cStringIO
import threading
import time

class Camera:
    thread = None
    frame = None
    last_access = 0

    def __init__(self):
        self.initialize()

    def initialize(self):
        if Camera.thread is None:
            Camera.thread = threading.Thread(target=self.getFrame)
            Camera.thread.start()

            while self.frame is None:
                time.sleep(0)

    def get_frame(self):
        Camera.last_access = time.time()
        self.initialize()
        return self.frame


    @classmethod
    def getFrame(cls):
        with open('/dev/video0', 'rb+', buffering=0) as vd:
            cp = v4l2_capability()
            fcntl.ioctl(vd, VIDIOC_QUERYCAP, cp)

            fmt = v4l2_format()
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
            fcntl.ioctl(vd, VIDIOC_G_FMT, fmt)  # get current settings
            fcntl.ioctl(vd, VIDIOC_S_FMT, fmt)  # set whatever default settings we got before

            parm = v4l2_streamparm()
            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
            parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME
            fcntl.ioctl(vd, VIDIOC_G_PARM, parm)
            fcntl.ioctl(vd, VIDIOC_S_PARM, parm)  # just got with the defaults

            req = v4l2_requestbuffers()
            req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
            req.memory = V4L2_MEMORY_MMAP
            req.count = 1  # nr of buffer frames
            fcntl.ioctl(vd, VIDIOC_REQBUFS, req)  # tell the driver that we want some buffers


            buffers = []

            for ind in range(req.count):
                # setup a buffer
                buf = v4l2_buffer()
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
                buf.memory = V4L2_MEMORY_MMAP
                buf.index = ind
                fcntl.ioctl(vd, VIDIOC_QUERYBUF, buf)

                mm = mmap.mmap(vd.fileno(), buf.length, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset=buf.m.offset)
                buffers.append(mm)

                # queue the buffer for capture
                fcntl.ioctl(vd, VIDIOC_QBUF, buf)


            buf_type = v4l2_buf_type(V4L2_BUF_TYPE_VIDEO_CAPTURE)
            fcntl.ioctl(vd, VIDIOC_STREAMON, buf_type)


            t0 = time.time()
            max_t = 1
            ready_to_read, ready_to_write, in_error = ([], [], [])
            while len(ready_to_read) == 0 and time.time() - t0 < max_t:
                ready_to_read, ready_to_write, in_error = select.select([vd], [], [], max_t)

            while True:
                buf = v4l2_buffer()
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE
                buf.memory = V4L2_MEMORY_MMAP
                fcntl.ioctl(vd, VIDIOC_DQBUF, buf)  # get image from the driver queue
                mm = buffers[buf.index]
                middle_data = mm.read(buf.length)
                im = Image.open(cStringIO.StringIO(middle_data))
                img_buf = cStringIO.StringIO()
                im.save(img_buf,"JPEG",quality=75)
                mm.seek(0)
                fcntl.ioctl(vd, VIDIOC_QBUF, buf)  # requeue the buffer
                cls.frame = img_buf.getvalue()
                if( time.time() - cls.last_access > 10):
                        break
            buf_type = v4l2_buf_type(V4L2_BUF_TYPE_VIDEO_CAPTURE)
            fcntl.ioctl(vd, VIDIOC_STREAMOFF, buf_type)
            cls.thread = None

