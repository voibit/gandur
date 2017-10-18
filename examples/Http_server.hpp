

#ifndef HTTPSERVER_HPP_INCLUDED
#define HTTPSERVER_HPP_INCLUDED

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <opencv2/core/core.hpp>
#include <boost/thread.hpp>
using boost::asio::ip::tcp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;
class HttpServer
{
public:
  std::map<std::string, std::string> winnames;
  std::map<std::string,int> requestcounts;
  std::map<std::string,std::vector<unsigned char> > jpegbuffers;
  short port;
  HttpServer();
  void run(int portno);
  boost::shared_mutex mut;
  boost::condition_variable_any cond;
  int httpdelay;
  void IMSHOW(std::string win, cv::Mat mat);
  int compression;
  bool is_debug;

private:
  int it;
  void server(int port);
  void session(socket_ptr sock);

  void handleinfo(socket_ptr sock);
  void handlewindows(socket_ptr sock);
  void handlemjpeg(socket_ptr sock,std::string winname);
  void handlejpg(socket_ptr sock,std::string winname);
  void handle404(socket_ptr sock);
};

#endif