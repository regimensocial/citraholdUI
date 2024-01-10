#ifndef INTERNALSERVER_H
#define INTERNALSERVER_H

#include <QHttpServer>

class InternalServer
{
public:
    InternalServer();

    void startServer();
    int getSuccessfulPort() { return successfulPort; }



private:
    QHttpServer httpServer;
    int successfulPort = -1;



};

#endif // INTERNALSERVER_H
