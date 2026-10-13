#include "include/CServer.h"
#include "include/CSession.h"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>

int main() {
    boost::asio::io_context ioc;
    CServer a(ioc, (short)10086);
    ioc.run();
    return 0;
}
