#include <division.h>
#include <zip.h>
#include <stdio.h>
#include <dirent.h>
#include <thread>
#include <vector>
#include <fstream>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <atomic>

static const char *const HEADER = "\nZiper&Unziper © 20xx Smth Inc.\n\n";
static const char *const USAGE = "Usage:\n\tziper --zip <dirPath>\n\tziper --unzip <dirPath>\n\nDescription:\n\tCompresses, uncompresses files within a directory,\n\tand reports compression info in a csv file.\n";

struct dirent *de;

typedef struct mesg_buffer { 
  long mesg_type;
  char mesg_text[100];
};

std::atomic<bool> stopRecording;

int getFileSize(std::string fileName)
{
    ifstream in_file(fileName, ios::binary);
    in_file.seekg(0, ios::end);
    return in_file.tellg();
}

void recordStats(){
  FILE *fptr;
  fptr = fopen("ziper_stats.csv","w");
  fprintf(fptr, "%s", "File name,Original size,Compressed size,Compression ratio,Space savings\n");

  key_t key = ftok("progfile", 65);
  int msgid = msgget(key, 0666 | IPC_CREAT);
  mesg_buffer message;
  while (!stopRecording)
  {
    // msgrcv to receive message 
    msgrcv(msgid, &message, sizeof(message), 1, 0); 

    if(strlen(message.mesg_text) > 0){
      // display the message 
      printf("Data Received is : %s \n", message.mesg_text);
      fprintf(fptr, "%s \n", message.mesg_text);
    }
  }

  // to destroy the message queue 
  msgctl(msgid, IPC_RMID, NULL);
  // statsFile.close();
  fclose(fptr);
}

void zipFile(std::string dirPath, std::string fileName)
{
    struct zip_t *zip = zip_open((dirPath + "/" + fileName + ".zip").c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    static const std::string separator = "/";
    zip_entry_open(zip, fileName.c_str());
    zip_entry_fwrite(zip, (dirPath + separator + fileName).c_str());
    zip_entry_close(zip);
    zip_close(zip);

    std::string toSend = fileName + ",";

    int originalSize = getFileSize(dirPath + separator + fileName);
    int compressedSize = getFileSize(dirPath + "/" + fileName + ".zip");
    float compressionRatio = (float)originalSize / (float)compressedSize;
    float spaceSavings = 1 - (float)compressedSize / (float)originalSize;

    toSend += std::to_string(originalSize) + "," + std::to_string(compressedSize) + "," + 
              std::to_string(compressionRatio) + "," + std::to_string(spaceSavings); 

    // Send stats through message queue
    mesg_buffer message;
    key_t key = ftok("progfile", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;
    strcpy(message.mesg_text, toSend.c_str());
    msgsnd(msgid, &message, sizeof(message), 0);
    printf("Data sent is : %s \n", message.mesg_text);
}

int on_extract_entry(const char *filename, void *arg)
{
    static int i = 0;
    int n = *(int *)arg;
    printf("Extracted: %s (%d of %d)\n", filename, ++i, n);
    return 0;
}

int main(int argc, const char *argv[]) 
{
  std::cout << HEADER;
  if (argc < 2) 
  {
    std::cout << USAGE;
    return 1;
  }

  std::vector<std::thread> threads;
  std::thread recordThread(recordStats);
  recordThread.detach();
  std::string operation = argv[1];

  stopRecording = false;

  DIR *dr = opendir(argv[2]);
  if (dr == NULL)
  { 
      printf("Could not open current directory" ); 
      return 0; 
  }

  if(strcmp(argv[1], "--zip") == 0) 
  {
    while ((de = readdir(dr)) != NULL)
    {
      if(strcmp(de->d_name,".") != 0 && strcmp(de->d_name,"..") != 0)
      {
        std::string fileName = de->d_name;
        std::cout << "Adding : "<< de->d_name << " to zip."<< std::endl;
        threads.push_back(std::thread(zipFile, std::string(argv[2]), fileName));
      }
    }

    for (std::thread & th : threads)
    {
      if (th.joinable())
          th.join();
    }
    stopRecording = true;
  } else if(strcmp(argv[1], "--unzip") == 0){
      while ((de = readdir(dr)) != NULL)
      {
        if(strstr(de->d_name, ".zip") != NULL)
        {
          int arg = 2;
          zip_extract((std::string(argv[2]) + std::string("/") + std::string(de->d_name)).c_str(), "./tmp", 
           on_extract_entry, &arg);
        }
      }
  } else {
    std::cout << "Invalid parameter!"<< std::endl;
    std::cout << USAGE;
    return -1;
  }
  closedir(dr);
  return 0;
}
