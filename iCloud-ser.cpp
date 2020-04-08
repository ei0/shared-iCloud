#include<iostream>
#include<thread>
#include"iCloud-ser.h"

void tt(){
  iCloud_sys::NonHotCompress ncom(GZFILE_DIR,BACKUP_DIR);
  ncom.Start();
  return;
}
void tt1(){
  iCloud_sys::Server server;
  server.Start();
  return;
}
int main()
{
  if(boost::filesystem::exists(GZFILE_DIR)==false){
    boost::filesystem::create_directory(GZFILE_DIR);
  }
  if(boost::filesystem::exists(BACKUP_DIR)==false){
    boost::filesystem::create_directory(BACKUP_DIR);
  }
  std::thread thr(tt);//非热点
  std::thread thr_ser(tt1);//网络
  thr.join();
  thr_ser.join();
  return 0;
}
