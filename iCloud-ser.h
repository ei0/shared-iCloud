#include <sys/types.h>
#include <sys/stat.h>
#include<cstdio>
#include<string>
#include<vector>
#include<fstream>
#include<unordered_map>
#include<zlib.h>
#include<pthread.h>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include "httplib.h"
#include<iostream>
#define NONHOT_TIME 10//最后一次访问在10秒以外
#define INTERVAL_TIME 30 //非热点的检测每30秒
#define BACKUP_DIR "./backup/"//文件备份路径
#define GZFILE_DIR "./gzfile/"//压缩包存放路径
#define DATA_FILE "./list.backup"//数据管理模块的数据备份文件名称
namespace iCloud_sys{
class FileUtil{
public:
  static bool Read(const std::string &name,std::string *body)
  {
    std::ifstream fs(name,std::ios::binary);
    if(!fs.is_open()){
      std::cout<<"open file"<< name <<"failed"<<std::endl;
     return false; 
    }
    int64_t fsize = boost::filesystem::file_size(name);
    body->resize(fsize);
    fs.read(&(*body)[0],fsize);
    if(!fs.good()){
      std::cout<<"read file"<< name << "failed" << std::endl;
      return false;
    }
    fs.close();
    return true;
  }
 static bool Write(const std::string &name,const std::string &body)
 {
   std::ofstream ofs(name,std::ios::binary);
   if(!ofs.is_open()){
     std::cout<<"open file" << name << "failed" << std::endl;
     return false;
   }
   ofs.write(body.c_str(),body.size());
   if(!ofs.good()){
     std::cout<< "write file" << name << "failed" << std::endl;
     return false;
   }
   ofs.close();
   return true;
 }
};
class CompressUtil{
public:
  static bool Compress(const std::string& src,const std::string& dst)
  {
    std::string body;
    FileUtil::Read(src,&body);
    gzFile gf = gzopen(dst.c_str(),"wb");
    if(gf == NULL)
    {
      std::cout<<"open file"<< dst << "failed"<<std::endl;
      return false;
    }
    size_t cont = 0;
    while(cont<body.size())
    {
      int ret = gzwrite(gf,&body[cont],body.size()-cont);
      if(ret == 0)
      {
        std::cout<< "file" << dst <<"write compress data failed"<<std::endl;
        return false;
      }
      cont += ret;
    }
    gzclose(gf);
    return true;
  }
  static bool UnCompress(const std::string& src,const std::string& dst)
  {
    std::ofstream ofs(dst,std::ios::binary);
    if(!ofs.is_open())
    {
      std::cout<<"open file"<< dst << "failed"<<std::endl;
      ofs.close();
      return false;
    }
    gzFile gf = gzopen(src.c_str(),"rb");
    if(gf == NULL)
    {
      std::cout<< "open file" << src << "failed"<<std::endl;
      ofs.close();
      return false;
    }
    int ret;
    char tmp[4096] = {0};
    while((ret = gzread(gf,tmp,4096))>0)
    {
      ofs.write(tmp,ret);
    }
    if(ret == -1)
    {
      std::cout<<"file"<< src << "write UnCompress data failed" << std::endl;
      ofs.close();
      return false;
    }
    ofs.close();
    gzclose(gf);
    return true;
  }
};
class DataManager{
private:
  std::string m_back_file;
  std::unordered_map<std::string,std::string> m_file_list;
  pthread_rwlock_t m_rwlock;
public:
  DataManager(const std::string& path):m_back_file(path)
  {
    InitLoad();
    pthread_rwlock_init(&m_rwlock,NULL);
  }
  ~DataManager()
  {
    pthread_rwlock_destroy(&m_rwlock);
  }
  bool Exists(const std::string& name)
  {
    pthread_rwlock_rdlock(&m_rwlock);
    auto it = m_file_list.find(name);
    if(it == m_file_list.end())
    {
     pthread_rwlock_unlock(&m_rwlock); 
     return false;
    }
    pthread_rwlock_unlock(&m_rwlock);
    return true;
  }
  bool IsCompress(const std::string& name)
  {
    pthread_rwlock_rdlock(&m_rwlock);
    auto it = m_file_list.find(name);
    if(it == m_file_list.end())
    {
     pthread_rwlock_unlock(&m_rwlock); 
     return false;
    }
    if(it->first == it->second)
    {
     pthread_rwlock_unlock(&m_rwlock); 
     return false;
    } 
    pthread_rwlock_unlock(&m_rwlock);
    return true;
  }
  bool NonCompressList(std::vector<std::string> *list)
  {
    pthread_rwlock_rdlock(&m_rwlock);
    for(auto it = m_file_list.begin();it!=m_file_list.end();++it)
    {
      if(it->first == it->second)
      {
        list->push_back(it->first);
      }
    }
    pthread_rwlock_unlock(&m_rwlock);
    return true;
  }
  bool Insert(const std::string& src,const std::string& dst)
  {
    pthread_rwlock_wrlock(&m_rwlock);
    //std::cout<<"insert begin "<<"s_filename: "<<src<<"d_filename: "<<dst<<std::endl;
    m_file_list[src] = dst;
    pthread_rwlock_unlock(&m_rwlock);
    Storage();
    return true;
  }
  bool GetAllName( std::vector<std::string> *list)
  {
    pthread_rwlock_rdlock(&m_rwlock);
    for(auto it = m_file_list.begin();it!=m_file_list.end();++it)
    {
      list->push_back(it->first);
    }
    pthread_rwlock_unlock(&m_rwlock);
    return true;
  }
  bool GetGzName(const std::string& src,std::string *dst)
  {
    pthread_rwlock_rdlock(&m_rwlock);
    auto it = m_file_list.find(src);
    if(it == m_file_list.end())
    {
      pthread_rwlock_unlock(&m_rwlock);
      return false;
    }
    *dst = it->second;
    pthread_rwlock_unlock(&m_rwlock);
    return true;
  }
  bool Storage()
  {
    std::cout<<"storage begin" <<std::endl;
    std::stringstream tmp;
    pthread_rwlock_rdlock(&m_rwlock);
    for(auto it = m_file_list.begin();it!=m_file_list.end();++it)
    {
      tmp<<it->first<<' '<<it->second<<"\r\n";
    }
    pthread_rwlock_unlock(&m_rwlock);
    FileUtil::Write(m_back_file,tmp.str());
    return true;
  }
  bool InitLoad()
  {
    std::string body;
    if(!FileUtil::Read(m_back_file,&body))
    {
      return false;
    }
    std::vector<std::string> list;
    boost::split(list,body,boost::is_any_of("\r\n"),boost::token_compress_off);
    for(auto i:list)
    {
      size_t pos= i.find(" ");
      if(pos == std::string::npos){
        continue;
      }
      std::string key = i.substr(0,pos);
      std::string val = i.substr(pos+1);
      Insert(key,val);
    }
    return true;
  }
};
DataManager data_manage(DATA_FILE);
class NonHotCompress{
  private:
    std::string m_gz_dir;
    std::string m_bu_dir;
  private:
    bool FileIsHot(const std::string &name)
    {
      time_t cur_t = time(NULL);
      struct stat st;
      if(stat(name.c_str(),&st)<0){
        std::cout<<"get file "<<name<<" stat failed!"<<std::endl;
        return false;
      }
      if((cur_t-st.st_atime)>NONHOT_TIME){
        return false;
      }
      return true;
    }
  public:
    NonHotCompress(const std::string gz_dir,const std::string bu_dir):
    m_gz_dir(gz_dir),
    m_bu_dir(bu_dir)
    {}
    bool Start()
    {
      while(1){
        std::vector<std::string> list;
        data_manage.NonCompressList(&list);
        for(size_t i = 0;i<list.size();i++){
              std::string s_filename = list[i];
              std::string d_filename = list[i] + ".gz";
              std::string src_name = m_bu_dir + s_filename;
              std::string dst_name = m_gz_dir + d_filename;
            //std::cout<<"want to get file "<< list[i]<<" stat" <<std::endl;
            bool ret = FileIsHot(src_name);
            //std::cout<<"ret = " <<ret<<std::endl;
            if(!ret){
              //std::cout<<"yi jin ru  "<<std::endl;
              std::cout<<"Non hot file "<<list[i]<<std::endl;
              bool cur = CompressUtil::Compress(src_name,dst_name);
              //std::cout<< "compress false or true: " << cur << std::endl; 
              if(cur){
                //std::cout<<"jin ru "<<std::endl;
                data_manage.Insert(s_filename,d_filename);
               // std::cout<<"insert end "<<"s_filename: "<<s_filename<<"d_filename: "<<d_filename<<std::endl;
               // std::cout<<"unlink begin "<<"src_name: "<<src_name<<std::endl;
                unlink(src_name.c_str());
              } 
            }
        }
        sleep(INTERVAL_TIME);
      }
      return true;
    }
};
class Server{
  private:
    std::string m_file_dir;
    httplib::Server m_server;
    static void Upload(const httplib::Request &req,httplib::Response &rsp)
    {
      std::string filename = req.matches[1];
      std::string pathname = BACKUP_DIR + filename;
      FileUtil::Write(pathname,req.body);
      data_manage.Insert(filename,filename);
      rsp.status = 200;
      return;
    }
    static void List(const httplib::Request &req,httplib::Response &rsp)
    {
      std::vector<std::string> list;
      data_manage.GetAllName(&list);
      std::stringstream tmp;
      tmp << "<html><body><hr/>";
      for(size_t i =0;i<list.size();i++){
        tmp<<"<a href='/download/"<<list[i]<<"'>"<<list[i]<<"</a>";
        tmp<<"<hr />";
      }
      tmp<<"<hr/></body></html>";
      rsp.set_content(tmp.str().c_str(),tmp.str().size(),"text/html");
      rsp.status = 200;
      return;
    }
    static void Download(const httplib::Request &req,httplib::Response &rsp)
    {
      std::string filename = req.matches[1];
      if(!data_manage.Exists(filename))
      {
        rsp.status = 404;
        return;
      }
      std::string pathname = BACKUP_DIR + filename;
      if(data_manage.IsCompress(filename)){
        std::string gzfile;
        data_manage.GetGzName(filename,&gzfile);
        std::string gzpathname = GZFILE_DIR + gzfile;
        CompressUtil::UnCompress(gzpathname,pathname);
        unlink(gzpathname.c_str());
        data_manage.Insert(filename,filename);
      }

      FileUtil::Read(pathname,&rsp.body);
      rsp.set_header("Content-Type","application/octet-stream");
      rsp.status = 200;
    }
  public:
    Server(){
    }
    bool Start(){
      m_server.Put("/(.*)",Upload);
      m_server.Get("/list",List);
      m_server.Get("/download/(.*)",Download);
      m_server.listen("0.0.0.0",9001);
      return true;
    }
};
}
