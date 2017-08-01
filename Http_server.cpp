#include "Http_server.hpp"
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/format.hpp> 
#include <opencv2/opencv.hpp>
#include <boost/lexical_cast.hpp>

namespace bfs= boost::filesystem; 
using namespace std;
using boost::lexical_cast;

// Helper functions
#if defined(unix)        || defined(__unix)      || defined(__unix__) \
  || defined(linux)       || defined(__linux)     || defined(__linux__) \
  || defined(sun)         || defined(__sun) \
  || defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
  || defined(__FreeBSD__) || defined __DragonFly__ \
  || defined(sgi)         || defined(__sgi) \
  || defined(__MACOSX__)  || defined(__APPLE__) \
  || defined(__CYGWIN__)
#define is_nix
#endif

#if defined(_MSC_VER) || defined(WIN32)  || defined(_WIN32) || defined(__WIN32__) \
  || defined(WIN64)    || defined(_WIN64) || defined(__WIN64__)
#define is_win
#endif

#ifdef is_win 
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#endif

#ifdef is_nix 
#define SLEEP(ms) usleep(ms*1000)
#endif

std::vector<std::string> &dssplit(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> dssplit(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return dssplit(s, delim, elems);
}

void removeEmptyStrings(std::vector<std::string>& strings)
{
  std::vector<std::string>::iterator it = remove_if(strings.begin(), strings.end(),     mem_fun_ref(&std::string::empty));
  strings.erase(it, strings.end());
}

bool hasEnding(std::string const &fullString, std::string const &ending)
{
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
  }
  else {
    return false;
  }
}

bool startswith(std::string const &src, std::string const &start)
{

  if (src.compare(0, start.length(), start) == 0)
  {
    return true;
  }
  return false;
}

std::string urldecode(std::string &src) {
  std::string ret;
  char ch;
  int ii;
  for (size_t i = 0; i<src.length(); i++) {
    if (int(src[i]) == 37) {
      sscanf(src.substr(i + 1, 2).c_str(), "%x", &ii);
      ch = static_cast<char>(ii);
      ret += ch;
      i = i + 2;
    }
    else {
      ret += src[i];
    }
  }
  return (ret);
}

// Server implementation
HttpServer::HttpServer() :compression(70), is_debug(true), httpdelay(50)
{

}

void HttpServer::IMSHOW(std::string win, cv::Mat mat)
{


  winnames[win] = lexical_cast<string>(mat.cols) + "," + lexical_cast<string>(mat.rows);

  if (is_debug)
  {
    cv::imshow(win, mat);
  }
  else
  {
    //cvDestroyWindow(win.c_str());
  }

  if (requestcounts[win] > 0)
  {
    cv::Mat towrite;
    if (mat.type() == CV_8UC1)
    {
      cvtColor(mat, towrite, CV_GRAY2BGR);
    }
    else if (mat.type() == CV_32FC3)
    {
      double minVal, maxVal;
      minMaxLoc(mat, &minVal, &maxVal);
      mat.convertTo(towrite, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    }
    else{
      towrite = mat;
    }

    std::vector<uchar> buffer;
    std::vector<int> param(2);
    param[0] = CV_IMWRITE_JPEG_QUALITY;
    param[1] = compression;
    imencode(".jpg", towrite, buffer, param);
    jpegbuffers[win].swap(buffer);

  }
}
void HttpServer::run(int portno)
{
  port=portno;
  boost::thread t(boost::bind(&HttpServer::server,this,port));
}
void HttpServer::server(int port)
{
  try
  {

    boost::asio::io_service io_service;
    io_service.run();
    tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
    for (;;)
    {
      socket_ptr sock(new tcp::socket(io_service));
      a.accept(*sock);
      boost::thread t(boost::bind(&HttpServer::session, this, sock));
    }
  }
  catch (boost::exception & e)
  {
    std::cout << "OMG!" << boost::diagnostic_information(e)<<endl;
  }
}
void HttpServer::session(socket_ptr sock)
{
  try
  {
    boost::system::error_code ec;
    boost::asio::streambuf sbuffer;
    boost::asio::read_until(* sock, sbuffer, "\0", ec ); 
    const char* header=boost::asio::buffer_cast<const char*>(sbuffer.data());
    std::string reqStr(header,header+sbuffer.size());   
    sbuffer.consume(sbuffer.size());
    std::vector<std::string> strs;
    strs = dssplit(reqStr,' ');
    if(strs.size()>1)
    {
      std::string requesturl = urldecode(strs[1]);
      std::vector<std::string> splited=dssplit(requesturl,'/');
      removeEmptyStrings(splited);
      if(splited.size()==1)
      {
        if(startswith(splited[0],"windows"))
        {
          handlewindows(sock);
        }else if(startswith(splited[0],"info"))
        {
          handleinfo(sock);
        }else if(hasEnding(splited[0],".mjpg"))
        {
          handlemjpeg(sock,splited[0].substr(0,splited[0].size()-5));
        }
        /*
        else if(hasEnding(splited[0],".txt"))
        {
          handletxt(sock,splited[0].substr(0,splited[0].size()-4));
        }
        */
        else if(hasEnding(splited[0],".jpg") || splited[0].find(".jpg?")!=string::npos)
        {
          handlejpg(sock,splited[0]);
        }else
        {
          handle404(sock);
        }
      }else
      {
        handle404(sock);
      }
      sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

  }catch(const std::exception& ex)
  {
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint = sock->remote_endpoint(ec);
    if(!ec)
    {
      sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    }
    //DPRINTERR(ex.what());
  }catch(const  std::string& ex)
  {
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint = sock->remote_endpoint(ec);
    if(!ec)
    {
      sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    }
  }
}

void HttpServer::handleinfo(socket_ptr sock)
{

  boost::system::error_code error;
  boost::asio::streambuf sbuffer;
  std::ostream response_stream(&sbuffer);
  string retstr;
  for (std::map<std::string,std::string>::iterator it=winnames.begin(); it!=winnames.end(); ++it)
  {
    string wname =it->first;
    int rcnt = 0;
    if(requestcounts.find(wname)!=requestcounts.end())
    {
      rcnt=requestcounts[wname];
    }

    retstr+=boost::str(boost::format("{"
      "\"name\":\"%s\","
      "\"reqCnt\":%d,"
      "\"size\":\"%s\""
      "},"
      )
      %wname
      %rcnt
      %it->second
      );
  }
  if(retstr.size()>0) retstr.resize(retstr.size()-1);

  retstr=boost::str(boost::format("{"
    "\"windows\":[%s],"
    "\"version\":\"%s\","
    "\"fps\":%s"
    "}"
    )
    %retstr
    %"0.0"
    % to_string(0.)
    );
  response_stream << "HTTP/1.1 200 OK\r\n";
  response_stream << "Access-Control-Allow-Origin: *\r\n";
  response_stream << "Content-Type: text/plain\r\n\r\n";
  response_stream << retstr << "\r\n\r\n";

  boost::asio::write(*sock, sbuffer);
}
void HttpServer::handlewindows(socket_ptr sock)
{

  boost::system::error_code error;
  boost::asio::streambuf sbuffer;
  std::ostream response_stream(&sbuffer);
  string retstr;
  for (std::map<std::string,std::string>::iterator it=winnames.begin(); it!=winnames.end(); ++it)
  {
    string wname =it->first;
    int rcnt = 0;
    if(requestcounts.find(wname)!=requestcounts.end())
    {
      rcnt=requestcounts[wname];
    }

    retstr+=boost::str(boost::format("{"
      "\"name\":\"%s\","
      "\"reqCnt\":%d,"
      "\"size\":\"%s\""
      "},"
      )
      %wname
      %rcnt
      %it->second
      );
  }
  if(retstr.size()>0) retstr.resize(retstr.size()-1);

  retstr="{\"windows\":["+retstr+"]}";
  response_stream<<"HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n\r\n"<<
    retstr<<"\r\n\r\n";

  boost::asio::write(*sock, sbuffer);

}
void HttpServer::handlemjpeg(socket_ptr sock,std::string winname)
{
  if(requestcounts.find(winname)==requestcounts.end())
  {
    handle404(sock);
    return;
  }
  std::string frame=winname;
  //boost::shared_lock<boost::shared_mutex> lock(mut);
  //lock.lock();
  requestcounts[frame]++;
  //lock.unlock();
  boost::system::error_code error;
  boost::asio::streambuf sbuffer;
  std::ostream response_stream(&sbuffer);
  response_stream<<"HTTP/1.1 200 OK\r\n";
  response_stream<<"Content-Type: multipart/mixed;boundary=b\r\n";
  response_stream<<"Cache-Control: no-store\r\n";
  response_stream<<"Pragma: no-cache\r\n";
  response_stream<<"Audio Mode : None\r\n";
  response_stream<<"Connection: close\r\n";
  response_stream<<"\r\n";
  boost::asio::write(*sock, sbuffer); 
  for(;;)
  {
    try
    {
      if( (jpegbuffers.count(frame)<0 ||
        jpegbuffers[frame].size()<4) || 
        (jpegbuffers[frame][0]!=0xff && jpegbuffers[frame][1]!=0xd8 &&
        jpegbuffers[frame][jpegbuffers[frame].size()-2]!=0xff && jpegbuffers[frame][jpegbuffers[frame].    size()-1]!=0xd9))
      {

        SLEEP(10);
        continue;
      }
      //boost::shared_lock<boost::shared_mutex> lock(mut);
      response_stream<<"--b\r\n"; 
      response_stream<<"Content-Type: image/jpeg\r\n";
      response_stream<<"Content-length: "<<jpegbuffers[frame].size()<<"\r\n";
      response_stream<<"\r\n";
      boost::asio::write(*sock, sbuffer);
      boost::asio::write(*sock,boost::asio::buffer(jpegbuffers[frame], jpegbuffers[frame].size()));
      //lock.unlock();
      SLEEP(httpdelay);
    }
    catch (std::exception& e)
    {
      SLEEP(50);
      //lock.lock();
      requestcounts[frame]--;
      //lock.unlock();
      return;
    }
  }
  //lock.lock();
  requestcounts[frame]--;
  //lock.unlock();
}

void HttpServer::handlejpg(socket_ptr sock,std::string winname)
{
  if(winname.find("?")!=string::npos)
  {
    winname = winname.substr(0,winname.find("?"));
  }
  winname =winname.substr(0,winname.size()-4);
  std::string frame=winname;
  requestcounts[frame]++;
  boost::system::error_code error;
  boost::asio::streambuf sbuffer;
  std::ostream response_stream(&sbuffer);

  jpegbuffers[frame].clear();
  for(;;)
  {
    try
    {
      if( (jpegbuffers.count(frame)<0 ||
        jpegbuffers[frame].size()<4) || 
        (jpegbuffers[frame][0]!=0xff && jpegbuffers[frame][1]!=0xd8 &&
        jpegbuffers[frame][jpegbuffers[frame].size()-2]!=0xff && jpegbuffers[frame][jpegbuffers[frame].    size()-1]!=0xd9))
      {

        SLEEP(10);
        continue;
      }
      response_stream<<"HTTP/1.1 200 OK\r\n";
      response_stream<<"Content-Type:  image/jpeg\r\n";
      response_stream<<"Cache-Control: no-store\r\n";
      response_stream<<"Access-Control-Allow-Origin: *\r\n";
      response_stream<<"Pragma: no-cache\r\n";
      response_stream<<"Content-length: "<<jpegbuffers[frame].size()<<"\r\n";
      response_stream<<"Connection: close\r\n";

      response_stream<<"\r\n";
      boost::asio::write(*sock, sbuffer);
      boost::asio::write(*sock,boost::asio::buffer(jpegbuffers[frame], jpegbuffers[frame].size()));

      break;
    }
    catch (std::exception& e)
    {

      //DPRINTERR( "net exceptoin:"+std::string(e.what()));
      SLEEP(50);
      requestcounts[frame]--;
      return;
    }
  }
  requestcounts[frame]--;
}
void HttpServer::handle404(socket_ptr sock)
{


  boost::system::error_code error;
  boost::asio::streambuf sbuffer;
  std::ostream response_stream(&sbuffer);
  response_stream<<"HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "Content-Length: 132\r\n\r\n"
    "<html>\r\n"
    "<head><title>404 Not Found</title></head>\r\n"
    "<body bgcolor=\"white\">\r\n"
    "<center><h1>404 Not Found</h1></center>\r\n"   
    "</body>\r\n"
    "</html>\r\n";
  boost::asio::write(*sock, sbuffer);
}
/*
void HttpServer::handleTxt(socket_ptr sock,std::string winname)
{


  boost::system::error_code error;
  boost::asio::streambuf sbuffer;
  std::ostream response_stream(&sbuffer);
  response_stream<<"HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "Content-Length: 132\r\n\r\n"
    "<html>\r\n"
    "<head><title>404 Not Found</title></head>\r\n"
    "<body bgcolor=\"white\">\r\n"
    "<center><h1>404 Not Found</h1></center>\r\n"   
    "</body>\r\n"
    "</html>\r\n";
  boost::asio::write(*sock, sbuffer);
}

*/