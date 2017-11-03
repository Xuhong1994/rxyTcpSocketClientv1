from twisted.internet import reactor, protocol, endpoints
from twisted.protocols import basic

from twisted.internet import task

from camera_v4l2 import Camera

cam = Camera()

class PubProtocol(protocol.Protocol):
    def __init__(self, factory):
        self.factory = factory

    def connectionMade(self):
        self.factory.clients.add(self)

    def connectionLost(self, reason):
        self.factory.clients.remove(self)

    def dataReceived(self, data):
        pass

    def sendimage(self,data):
        self.transport.write(data)

class PubFactory(protocol.Factory):
    def __init__(self):
        self.clients = set()
        l = task.LoopingCall(self.test)
        l.start(0.03)

    def buildProtocol(self, addr):
        return PubProtocol(self)

    def test(self):
        if(len(self.clients) <= 0):
            return
        print "test"
        img = cam.get_frame()
        for c in self.clients:
            c.sendimage('666')
            c.sendimage("%010d" % len(img))
            c.sendimage(img)


endpoints.serverFromString(reactor, "tcp:1025").listen(PubFactory())
reactor.run()
