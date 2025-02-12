using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <string.h>

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

int imdb::cmp_fun_actors(const void * s1, const void * s2) {
    const elem *key = static_cast<const elem*>(s1);
    const char *first = static_cast<const char*>(key->itself);
    int offset = *(const int*)s2;
    const char *second = static_cast<const char*>(key->base) + offset;
    return strcmp(first, second); 
}

bool imdb::getCredits(const string& player, vector<film>& films) const { 
    int number_of_actors = *(int *)actorFile; 
    struct elem actor_info;

    actor_info.itself = (void *) player.c_str();
    actor_info.base = (void *) actorFile;

    void * actor_records = bsearch(&actor_info, (int *) actorFile + 1, number_of_actors, sizeof(int), cmp_fun_actors);
    
    if (actor_records == NULL) return false;

    char * records_ptr = (char *) actorFile + *(int *) actor_records;

    // Process the actor's information and movies details
    return process_actor_records(records_ptr, player, films);
}

bool imdb::process_actor_records(char* records_ptr, const string& player, vector<film>& films) const {
    int bytes_count = 0;

    if (player.size() % 2 == 0) {
        records_ptr += (player.size() + 2);
        bytes_count += (player.size() + 2);
    } else {
        records_ptr += (player.size() + 1);
        bytes_count += (player.size() + 1);
    }

    short number_of_movies = *(short *)records_ptr;
    bytes_count += 2;  
    records_ptr += 2;

    if (bytes_count % 4 != 0) records_ptr += 2;

    return extract_movie_details(records_ptr, number_of_movies, films);
}
//processing movies details
bool imdb::extract_movie_details(char* records_ptr, short number_of_movies, vector<film>& films) const {
    for (short i = 0; i < number_of_movies; i++) {
        struct film actor_film;
        char * movie_ptr = (char *) movieFile + *(int *)records_ptr;
        
        string name;
        while (*movie_ptr != '\0') {
            name += *movie_ptr;
            movie_ptr++;
        }
        movie_ptr++;
        records_ptr += 4;
        
        int movie_year = 1900 + *movie_ptr;
        actor_film.title = name;
        actor_film.year = movie_year;
        films.push_back(actor_film);
    }
    return true;
}

int imdb::cmp_fun_movies(const void * s1 , const void *s2){
    const elem *movie1_info = static_cast<const elem *>(s1);
    const char *movie2_ptr =static_cast<const char*>(movie1_info->base)+*(const int*)s2;
    const film *movie1 = static_cast<const film*>(movie1_info->itself);
    string movie2_name(movie2_ptr);
    movie2_ptr +=movie2_name.length();
    movie2_ptr++;
    int year = *(movie2_ptr);
    struct film movie2;
    movie2.title = movie2_name;
    movie2.year = year + 1900;
    if (*movie1 < movie2) return -1;  
    else if (*movie1 == movie2) return 0;  
    else return 1;  
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
  int number_of_movies =  *(int *)movieFile;
  struct elem movie_info;
  movie_info.base = (void *) movieFile;
  movie_info.itself = (void *)&movie;
  void *movie_records = bsearch(&movie_info,(int *)movieFile+1,number_of_movies,sizeof(int),cmp_fun_movies);
  if(movie_records==NULL){
    return false;
  }
  int byte_count = 0 ;
  char *records_ptr = (char *) movieFile + *(int *)movie_records;  
  records_ptr +=movie.title.length();//move by name
  records_ptr++;//move by \0
  records_ptr++;//move by one byte which was for year
  byte_count +=movie.title.length()+2;
  if(byte_count%2==1){
    records_ptr++;//move extra \0
    byte_count++;
  }
  //read short for number of actros
  short number_of_actros = *(short *)(records_ptr);
  byte_count+=2;
  records_ptr+=2;
  if(byte_count%4!=0){
    records_ptr+=2;
  }
  for(short i = 0 ; i < number_of_actros ; i ++){
    int offset = *(int *) records_ptr;
    records_ptr+=4;
    char * actro_name_ptr = (char *)actorFile + offset;
    string actor_name(actro_name_ptr);
    players.push_back(actor_name);
  }
  return true;   
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
