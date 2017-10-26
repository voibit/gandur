#include <opencv2/opencv.hpp>
#include "http_server.hpp"
#include <iostream>
#include <fstream>

using namespace cv;
#define MJPGFILE_BUFFER_SIZE 10240
class MjpgFileCapture{
public:
  static double lastframeseen;
  MjpgFileCapture() {};
  MjpgFileCapture(std::string filepath)
  {
    filepath_ = filepath;
    is_inited_ = false;
    skip_ = true;
    imgready_ = false;
    ff_ = false;
    readbytes_ = -2;
    i_ = 0;
  };

  void init();
  MjpgFileCapture& operator >>(cv::Mat& out);
private:
  std::string filepath_;
  bool is_inited_;
  std::ifstream ifstream_;
  std::vector<char> data_;
  bool skip_;
  bool imgready_;
  bool ff_;//have we seen ff byte?
  long long readbytes_;
  char ca_[MJPGFILE_BUFFER_SIZE];
  int i_;//loop index
};

void MjpgFileCapture::init()
{
  is_inited_ = true;
  ifstream_ = std::ifstream(filepath_.c_str(), std::ios::binary);
}

MjpgFileCapture& MjpgFileCapture::operator >> (cv::Mat& out)
{
  out = Mat();
  if (!is_inited_)
  {
    init();
  }


  while (1)
  {
    uchar c;
    if (readbytes_ != 0 && readbytes_ != -1)
    {
      if (i_ >= readbytes_)
      {
        ifstream_.read(ca_, MJPGFILE_BUFFER_SIZE);
        readbytes_ = ifstream_.gcount();
        i_ = 0;
      }
      for (; i_ < readbytes_; i_++)
      {
        c = ca_[i_];
        if (ff_ && c == 0xd8)
        {
          skip_ = false;
          data_.push_back((uchar)0xff);
        }
        if (ff_ && c == 0xd9)
        {
          imgready_ = true;
          data_.push_back((uchar)0xd9);
          skip_ = true;
        }
        ff_ = c == 0xff;
        if (!skip_)
        {
          data_.push_back(c);
        }
        if (imgready_)
        {
          if (data_.size() != 0)
          {
            cv::Mat data_mat(data_);
            cv::Mat frame(imdecode(data_mat, 1));
            out = frame;
          }
          else
          {
            printf("warning:image is ready and data is empty. Likely bug.");
          }
          imgready_ = false;
          skip_ = true;
          data_.clear();
          return *this;
        }
      }
    }
    else
    {
      //own exception class
      throw std::string("zero byte read:probably end of file.");
    }
  }
  return *this;
}
HttpServer* server = 0;
void file_loop()
{

  MjpgFileCapture cap("C:/v/frame.mjpg");
  while (true)
  {
    Mat im;
    cap >> im;
    server->IMSHOW("im", im);
    imshow("im", im);
    if (waitKey(1) == 27)
      exit(0);
  }
}
int main(int argc, char** argv)
{
  server = new HttpServer;
  //server->port = 8080;
  server->run(8080);
  while (true)
  {
    try{
      file_loop();
    }
    catch (...)
    {

    }
  }
  return 0;
}    