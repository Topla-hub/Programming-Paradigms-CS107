#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "bool.h"
#include "html-utils.h"
#include "streamtokenizer.h"
#include "url.h"
#include "urlconnection.h"
#include "hashset.h"

#define NUM_BUCKETS_STOP 1009
#define NUM_BUCKETS_DATA 10007

typedef struct {
  char *word;
  vector *freq_vec;
} Word_Freqs_IN_Newses;

typedef struct {
  char *adress;
  char *name;
  int count;
} News;

static void Welcome(const char *welcomeTextFileName);
static void BuildIndices(const char *feedsFileName, hashset *stop_words, hashset *words_from_newses);
static void ProcessFeed(const char *remoteDocumentName, hashset *stop_words, hashset *words_from_newses);
static void PullAllNewsItems(urlconnection *urlconn, hashset *stop_words, hashset *words_from_newses);
static bool GetNextItemTag(streamtokenizer *st);
static void ProcessSingleNewsItem(streamtokenizer *st, hashset *stop_words, hashset *words_from_newses);
static void ExtractElement(streamtokenizer *st, const char *htmlTag, char dataBuffer[], int bufferLength);
static void ParseArticle(const char *articleTitle, const char *articleDescription, const char *articleURL, 
                         hashset *stop_words, hashset *words_from_newses);
static void ScanArticle(streamtokenizer *st, const char *articleTitle, const char *unused, const char *articleURL, 
                        hashset *stop_words, hashset *words_from_newses);
static void QueryIndices(hashset *stop_words, hashset *words_from_newses);
static void ProcessResponse(const char *word, hashset *stop_words, hashset *words_from_newses);
static bool WordIsWellFormed(const char *word);

/**
 * Function: main
 * --------------
 * Serves as the entry point of the full application.
 * You'll want to update main to declare several hashsets--
 * one for stop words, another for previously seen urls, etc--
 * and pass them (by address) to BuildIndices and QueryIndices.
 * In fact, you'll need to extend many of the prototypes of the
 * supplied helpers functions to take one or more hashset *s.
 *
 * Think very carefully about how you're going to keep track of
 * all of the stop words, how you're going to keep track of
 * all the previously seen articles, and how you're going to
 * map words to the collection of news articles where that
 * word appears.
 */

static const char *const kWelcomeTextFile = "data/welcome.txt";
static const char *const kDefaultFeedsFile = "data/rss-feeds.txt";
static const char *const kStopWords = "data/stop-words.txt";
static const char *const kFilePrefix = "file://";
static const char *const kTextDelimiters = " \t\n\r\b!@$%^*()_+={[}]|\\'\":;/?.>,<~`";

void store_stop_words(const char *stopWordsfile, hashset* stop_words) {
  FILE *infile;
  streamtokenizer token;
  char buffer[1024];

  infile = fopen(stopWordsfile, "r");
  assert(infile != NULL);

  STNew(&token, infile, kTextDelimiters, true);
  while(STNextToken(&token, buffer, sizeof(buffer))) {
    char *cpy = strdup(buffer);
    HashSetEnter(stop_words, &cpy);
  }

  STDispose(&token);
  fclose(infile);
}

static const signed long kHashMultiplier = -1664117991L;
static int StringHash(const void *s, int numBuckets)  
{            
  int i;
  unsigned long hashcode = 0;
  for (i = 0; i < strlen(*(char**)s); i++) 
    hashcode = hashcode * kHashMultiplier + tolower( (*(char **)s)[i] );
  return hashcode % numBuckets;                                
}

void string_free_fn(void* elem) { 
  free(*(char **)elem); }

int string_compare_fn(const void *a, const void *b) {
  return strcasecmp(*(char **)a, *(char **)b);
}

int vector_compare_fn(const void *n1, const void *n2) {
  int c1 = ((News *)n1)->count;
  int c2 = ((News *)n2)->count;
  if (c1 > c2) {
    return -1;
  } else if (c1 < c2) {
    return 1;
  } else {
    return 0;
  }
}

void vector_free_fn(void *elem) {
  News *news =  elem;
  free(news->name);
  free(news->adress);
}

void hashmap_free_fn(void *elem) {
  Word_Freqs_IN_Newses *freq = (Word_Freqs_IN_Newses *)elem;
  char *word = freq->word;
  vector *freq_vec = freq->freq_vec;
  free(word);
  VectorDispose(freq_vec);
  free(freq_vec);
}

void add_freq(char *word, const char *articleTitle, const char *articleURL, hashset *stop_words, hashset *words_from_newses) {
  if (HashSetLookup(stop_words, &word) != NULL) return;
  void *data = HashSetLookup(words_from_newses, &word);
  vector *newses;
  if (data == NULL) {
    newses = malloc(sizeof(vector));
    VectorNew(newses, sizeof(News), vector_free_fn, 4);
    News news;
    news.adress = strdup(articleURL);
    news.name = strdup(articleTitle);
    news.count = 1; 
    VectorAppend(newses, &news);
    Word_Freqs_IN_Newses freqs;
    freqs.word = strdup(word);
    freqs.freq_vec = newses; 
    HashSetEnter(words_from_newses, &freqs);
    return;
  } 
  newses = ((Word_Freqs_IN_Newses *)data)->freq_vec;
  url URL, vectorURL;
  URLNewAbsolute(&URL, articleURL);
  for (int i = 0; i < VectorLength(newses); i++) {
    News *news = (News *)VectorNth(newses, i);
    URLNewAbsolute(&vectorURL, news->adress);
    if (!strcasecmp(articleURL, news->adress) || 
    (strcasecmp(URL.serverName, vectorURL.serverName) && !strcasecmp(articleTitle, news->name))) 
    {
      news->count++;
      URLDispose(&vectorURL);
      URLDispose(&URL);
      return;
    }
    URLDispose(&vectorURL);
  }
  News news;
  news.adress = strdup(articleURL);
  news.name = strdup(articleTitle);
  news.count = 1; 
  VectorAppend(newses, &news);  
  URLDispose(&URL);
}



int main(int argc, char **argv) {
  setbuf(stdout, NULL);
  curl_global_init(CURL_GLOBAL_DEFAULT);

  Welcome(kWelcomeTextFile);
  
  hashset stop_words;
  HashSetNew(&stop_words, sizeof(char *), NUM_BUCKETS_STOP, StringHash, string_compare_fn, string_free_fn);
  store_stop_words(kStopWords, &stop_words);
  
  hashset words_from_newses;
  HashSetNew(&words_from_newses, sizeof(Word_Freqs_IN_Newses), NUM_BUCKETS_DATA, StringHash, string_compare_fn, hashmap_free_fn);
  
  BuildIndices((argc == 1) ? kDefaultFeedsFile : argv[1], &stop_words, &words_from_newses);
  QueryIndices(&stop_words, &words_from_newses);
  
  HashSetDispose(&stop_words);
  HashSetDispose(&words_from_newses);
  
  curl_global_cleanup();
  return 0;
}

size_t SavePage(char *ptr, size_t size, size_t nmemb, void *data) {
  return fprintf((FILE *)data, "%s", ptr);
}

static FILE *RemoveCData(const char *tmpFile) {
  FILE *inp = fopen(tmpFile, "rb");
  fseek(inp, 0, SEEK_END);
  long fsize = ftell(inp);
  fseek(inp, 0, SEEK_SET); /* same as rewind(f); */
  char *contents = malloc(fsize + 1);
  long read = fread(contents, 1, fsize, inp);
  assert(fsize == read);
  fclose(inp);
  FILE *out = fopen(tmpFile, "w");
  bool inside_cdata = false;
  for (int i = 0; i < fsize; ++i) {
    if (strncasecmp(contents + i, "<![CDATA[", strlen("<![CDATA[")) == 0) {
      inside_cdata = true;
      i += strlen("<![CDATA[") - 1;
    } else if (inside_cdata && strncmp(contents + i, "]]>", 3) == 0) {
      inside_cdata = false;
      i += 2;
    } else {
      fprintf(out, "%c", contents[i]);
    }
  }
  fclose(out);
  free(contents);
  return fopen(tmpFile, "r");
}

static FILE *FetchURL(const char *path, const char *tmpFile) {
  FILE *tmpDoc = fopen(tmpFile, "w");
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
  curl_easy_setopt(curl, CURLOPT_URL, path);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SavePage);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, tmpDoc);
  res = curl_easy_perform(curl);
  fclose(tmpDoc);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    return NULL;
  }
  return RemoveCData(tmpFile);
}

/**
 * Function: Welcome
 * -----------------
 * Displays the contents of the specified file, which
 * holds the introductory remarks to be printed every time
 * the application launches.  This type of overhead may
 * seem silly, but by placing the text in an external file,
 * we can change the welcome text without forcing a recompilation and
 * build of the application.  It's as if welcomeTextFileName
 * is a configuration file that travels with the application.
 */

static const char *const kNewLineDelimiters = "\r\n";
static void Welcome(const char *welcomeTextFileName) {
  FILE *infile;
  streamtokenizer token;
  char buffer[1024];

  infile = fopen(welcomeTextFileName, "r");
  assert(infile != NULL);

  STNew(&token, infile, kNewLineDelimiters, true);
  while (STNextToken(&token, buffer, sizeof(buffer))) {
    printf("%s\n", buffer);
  }

  printf("\n");
  STDispose(&token); // remember that STDispose doesn't close the file, since STNew doesn't open one..
  fclose(infile);
}
/**
 * Function: BuildIndices
 * ----------------------
 * As far as the user is concerned, BuildIndices needs to read each and every
 * one of the feeds listed in the specied feedsFileName, and for each feed parse
 * content of all referenced articles and store the content in the hashset of
 * indices. Each line of the specified feeds file looks like this:
 *
 *   <feed name>: <URL of remore xml document>
 *
 * Each iteration of the supplied while loop parses and discards the feed name
 * (it's in the file for humans to read, but our aggregator doesn't care what
 * the name is) and then extracts the URL.  It then relies on ProcessFeed to
 * pull the remote document and index its content.
 */

static void BuildIndices(const char *feedsFileName, hashset *stop_words, hashset *words_from_newses) {
  FILE *infile;
  streamtokenizer token;
  char file_name[1024];

  infile = fopen(feedsFileName, "r");
  assert(infile != NULL);
  STNew(&token, infile, kNewLineDelimiters, true);
  while (STSkipUntil(&token, ":") != EOF) { // ignore everything up to the first selicolon of the line
    STSkipOver( &token, ": "); // now ignore the semicolon and any whitespace directly after it
    STNextToken(&token, file_name, sizeof(file_name));
    ProcessFeed(file_name, stop_words, words_from_newses);
  }

  STDispose(&token);
  fclose(infile);
  printf("\n");
}

/** * Function: ProcessFeedFromFile * --------------------- * ProcessFeed
 * locates the specified RSS document, from locally */

static void ProcessFeedFromFile(char *fileName, hashset *stop_words, hashset *words_from_newses) {
  FILE *infile;
  streamtokenizer token;
  char news[1024];
  news[0] = '\0';
  infile = fopen((const char *)fileName, "r");
  assert(infile != NULL);
  STNew(&token, infile, kTextDelimiters, true);
  ScanArticle(&token, (const char *)fileName, news, (const char *)fileName, stop_words, words_from_newses);
  STDispose(&token); // remember that STDispose doesn't close the file, since STNew doesn't open one..
  fclose(infile);
}

/**
 * Function: ProcessFeed
 * ---------------------
 * ProcessFeed locates the specified RSS document, and if a (possibly
 * redirected) connection to that remote document can be established, then
 * PullAllNewsItems is tapped to actually read the feed.  Check out the
 * documentation of the PullAllNewsItems function for more information, and
 * inspect the documentation for ParseNews for information about what the
 * different response codes mean.
 */

static void ProcessFeed(const char *remoteDocumentName, hashset *stop_words, hashset *words_from_newses) {

  if (!strncmp(kFilePrefix, remoteDocumentName, strlen(kFilePrefix))) {
    ProcessFeedFromFile((char *)remoteDocumentName + strlen(kFilePrefix), stop_words, words_from_newses);
    return;
  }

  url u;
  urlconnection urlconn;

  URLNewAbsolute(&u, remoteDocumentName);
  URLConnectionNew(&urlconn, &u);

  switch (urlconn.responseCode) {
  case 0:
    printf("Unable to connect to \"%s\".  Ignoring...", u.serverName);
    break;
  case 200:
    PullAllNewsItems(&urlconn, stop_words, words_from_newses);
    break;
  case 301:
  case 302:
    ProcessFeed(urlconn.newUrl, stop_words, words_from_newses);
    break;
  default:
    printf(
        "Connection to \"%s\" was established, but unable to retrieve \"%s\". [response code: %d, response message:\"%s\"]\n",
        u.serverName, u.fileName, urlconn.responseCode, urlconn.responseMessage);
    break;
  };

  URLConnectionDispose(&urlconn);
  URLDispose(&u);
}

/**
 * Function: PullAllNewsItems
 * --------------------------
 * Steps though the data of what is assumed to be an RSS feed identifying the
 * names and URLs of online news articles.  Check out
 * "datafiles/sample-rss-feed.txt" for an idea of what an RSS feed from the
 * www.nytimes.com (or anything other server that syndicates is stories).
 *
 * PullAllNewsItems views a typical RSS feed as a sequence of "items", where
 * each item is detailed using a generalization of HTML called XML.  A typical
 * XML fragment for a single news item will certainly adhere to the format of
 * the following example:
 *
 * <item>
 *   <title>At Installation Mass, New Pope Strikes a Tone of Openness</title>
 *   <link>http://www.nytimes.com/2005/04/24/international/worldspecial2/24cnd-pope.html</link>
 *   <description>The Mass, which drew 350,000 spectators, marked an important
 * moment in the transformation of Benedict XVI.</description> <author>By IAN
 * FISHER and LAURIE GOODSTEIN</author> <pubDate>Sun, 24 Apr 2005 00:00:00
 * EDT</pubDate> <guid
 * isPermaLink="false">http://www.nytimes.com/2005/04/24/international/worldspecial2/24cnd-pope.html</guid>
 * </item>
 *
 * PullAllNewsItems reads and discards all characters up through the opening
 * <item> tag (discarding the <item> tag as well, because once it's read and
 * indentified, it's been pulled,) and then hands the state of the stream to
 * ProcessSingleNewsItem, which handles the job of pulling and analyzing
 * everything up through and including the </item> tag. PullAllNewsItems
 * processes the entire RSS feed and repeatedly advancing to the next <item> tag
 * and then allowing ProcessSingleNewsItem do process everything up until
 * </item>.
 */

static void PullAllNewsItems(urlconnection *urlconn, hashset *stop_words, hashset *words_from_newses) {
  streamtokenizer token;
  STNew(&token, urlconn->dataStream, kTextDelimiters, false);
  while (GetNextItemTag(&token)) { // if true is returned, then assume that <item ...> has just been
                                // read and pulled from the data stream
    ProcessSingleNewsItem(&token, stop_words, words_from_newses);
  }

  STDispose(&token);
}

/**
 * Function: GetNextItemTag
 * ------------------------
 * Works more or less like GetNextTag below, but this time
 * we're searching for an <item> tag, since that marks the
 * beginning of a block of HTML that's relevant to us.
 *
 * Note that each tag is compared to "<item" and not "<item>".
 * That's because the item tag, though unlikely, could include
 * attributes and perhaps look like any one of these:
 *
 *   <item>
 *   <item rdf:about="Latin America reacts to the Vatican">
 *   <item requiresPassword=true>
 *
 * We're just trying to be as general as possible without
 * going overboard.  (Note that we use strncasecmp so that
 * string comparisons are case-insensitive.  That's the case
 * throughout the entire code base.)
 */

static const char *const kItemTagPrefix = "<item";
static bool GetNextItemTag(streamtokenizer *st) {
  char htmlTag[1024];
  while (GetNextTag(st, htmlTag, sizeof(htmlTag))) {
    if (strncasecmp(htmlTag, kItemTagPrefix, strlen(kItemTagPrefix)) == 0) {
      return true;
    }
  }
  return false;
}

/**
 * Function: ProcessSingleNewsItem
 * -------------------------------
 * Code which parses the contents of a single <item> node within an RSS/XML
 * feed. At the moment this function is called, we're to assume that the <item>
 * tag was just read and that the streamtokenizer is currently pointing to
 * everything else, as with:
 *
 *      <title>Carrie Underwood takes American Idol Crown</title>
 *      <description>Oklahoma farm girl beats out Alabama rocker Bo Bice and
 * 100,000 other contestants to win competition.</description>
 *      <link>http://www.nytimes.com/frontpagenews/2841028302.html</link>
 *   </item>
 *
 * ProcessSingleNewsItem parses everything up through and including the </item>,
 * storing the title, link, and News description in local buffers long enough
 * so that the online new News identified by the link can itself be parsed
 * and indexed.  We don't rely on <title>, <link>, and <description> coming in
 * any particular order.  We do asssume that the link field exists (although we
 * can certainly proceed if the title and News descrption are missing.) There
 * are often other tags inside an item, but we ignore them.
 */

static const char *const kItemEndTag = "</item>";
static const char *const kTitleTagPrefix = "<title";
static const char *const kDescriptionTagPrefix = "<description";
static const char *const kLinkTagPrefix = "<link";
static void ProcessSingleNewsItem(streamtokenizer *st, hashset *stop_words, hashset *words_from_newses){
  char htmlTag[1024];
  char articleTitle[1024];
  char articleDescription[1024];
  char articleURL[1024];
  articleTitle[0] = articleDescription[0] = articleURL[0] = '\0';

  while (GetNextTag(st, htmlTag, sizeof(htmlTag)) &&(strcasecmp(htmlTag, kItemEndTag) != 0)) {
    if (strncasecmp(htmlTag, kTitleTagPrefix, strlen(kTitleTagPrefix)) == 0)
      ExtractElement(st, htmlTag, articleTitle, sizeof(articleTitle));
    if (strncasecmp(htmlTag, kDescriptionTagPrefix,strlen(kDescriptionTagPrefix)) == 0)
      ExtractElement(st, htmlTag, articleDescription,sizeof(articleDescription));
    if (strncasecmp(htmlTag, kLinkTagPrefix, strlen(kLinkTagPrefix)) == 0)
      ExtractElement(st, htmlTag, articleURL, sizeof(articleURL));
  }

  if (strncmp(articleURL, "", sizeof(articleURL)) == 0)
    return; 
  ParseArticle(articleTitle, articleDescription, articleURL, stop_words, words_from_newses);
}
/**
 * Function: ExtractElement
 * ------------------------
 * Potentially pulls text from the stream up through and including the matching
 * end tag.  It assumes that the most recently extracted HTML tag resides in the
 * buffer addressed by htmlTag.  The implementation populates the specified data
 * buffer with all of the text up to but not including the opening '<' of the
 * closing tag, and then skips over all of the closing tag as irrelevant.
 * Assuming for illustration purposes that htmlTag addresses a buffer containing
 * "<description" followed by other text, these three scanarios are handled:
 *
 *    Normal Situation:
 * <description>http://some.server.com/someRelativePath.html</description>
 *    Uncommon Situation:   <description></description>
 *    Uncommon Situation:   <description/>
 *
 * In each of the second and third scenarios, the document has omitted the data.
 * This is not uncommon for the description data to be missing, so we need to
 * cover all three scenarious (I've actually seen all three.) It would be quite
 * unusual for the title and/or link fields to be empty, but this handles those
 * possibilities too.
 */

static void ExtractElement(streamtokenizer *st, const char *htmlTag, char dataBuffer[], int bufferLength) {
  assert(htmlTag[strlen(htmlTag) - 1] == '>');
  if (htmlTag[strlen(htmlTag) - 2] == '/')
    return;
  STNextTokenUsingDifferentDelimiters(st, dataBuffer, bufferLength, "<");
  RemoveEscapeCharacters(dataBuffer);
  if (dataBuffer[0] == '<')
    strcpy(dataBuffer, ""); 
  STSkipUntil(st, ">");
  STSkipOver(st, ">");
}

/**
 * Function: ParseArticle
 * ----------------------
 * Attempts to establish a network connect to the news News identified by the
 * three parameters.  The network connection is either established of not.  The
 * implementation is prepared to handle a subset of possible (but by far the
 * most common) scenarios, and those scenarios are categorized by response code:
 *
 *    0 means that the server in the URL doesn't even exist or couldn't be
 * contacted. 200 means that the document exists and that a connection to that
 * very document has been established. 301 means that the document has moved to
 * a new location 302 also means that the document has moved to a new location
 *    4xx and 5xx (which are covered by the default case) means that either
 *        we didn't have access to the document (403), the document didn't exist
 * (404), or that the server failed in some undocumented way (5xx).
 *
 * The are other response codes, but for the time being we're punting on them,
 * since no others appears all that often, and it'd be tedious to be fully
 * exhaustive in our enumeration of all possibilities.
 */

static void ParseArticle(const char *articleTitle, const char *articleDescription, const char *articleURL, 
                         hashset *stop_words, hashset *words_from_newses) {
  FILE *tmpDoc = FetchURL(articleURL, "tmp_doc");
  if (tmpDoc == NULL) {
    printf("Unable to fetch URL: %s\n", articleURL);
    return;
  }
  printf("Scanning \"%s\"\n", articleURL);
  streamtokenizer token;
  STNew(&token, tmpDoc, kTextDelimiters, false);
  ScanArticle(&token, articleTitle, articleDescription, articleURL, stop_words, words_from_newses);
  STDispose(&token);
  fclose(tmpDoc);
}

/**
 * Function: ScanArticle
 * ---------------------
 * Parses the specified article, skipping over all HTML tags, and counts the
 * numbers of well-formed words that could potentially serve as keys in the set
 * of indices. Once the full News has been scanned, the number of well-formed
 * words is printed, and the longest well-formed word we encountered along the
 * way is printed as well.
 *
 * This is really a placeholder implementation for what will ultimately be
 * code that indexes the specified content.
 */

static void ScanArticle(streamtokenizer *st, const char *articleTitle, const char *unused, const char *articleURL, 
                        hashset *stop_words, hashset *words_from_newses) {
  int count = 0;
  char word[1024];
  char longest_word[1024] = {'\0'};

  while (STNextToken(st, word, sizeof(word))) {
    if (strcasecmp(word, "<") == 0) {
      SkipIrrelevantContent(st); // in html-utls.h
    } else {
      RemoveEscapeCharacters(word);
      if (WordIsWellFormed(word)) {
        add_freq(word, articleTitle, articleURL, stop_words, words_from_newses);
        count++;
        if (strlen(word) > strlen(longest_word))
          strcpy(longest_word, word);
      }
    }
  }

  printf("\tWe counted %d well-formed words [including duplicates].\n", count);
  printf("\tThe longest word scanned was \"%s\".", longest_word);
  if (strlen(longest_word) >= 15 && (strchr(longest_word, '-') == NULL))
    printf(" [Ooooo... long word!]");
  printf("\n");
}

/**
 * Function: QueryIndices
 * ----------------------
 * Standard query loop that allows the user to specify a single search term, and
 * then proceeds (via ProcessResponse) to list up to 10 articles (sorted by
 * relevance) that contain that word.
 */

static void QueryIndices(hashset *stop_words, hashset *words_from_newses) {
  char response[1024];
  while (true) {
    // printf("Please enter a single query term that might be in our set of indices [enter to quit]: ");
    fgets(response, sizeof(response), stdin);
    response[strlen(response) - 1] = '\0';
    if (strcasecmp(response, "") == 0)
      break;
    ProcessResponse(response, stop_words, words_from_newses);
  }
}

/**
 * Function: ProcessResponse
 * -------------------------
 * Placeholder implementation for what will become the search of a set of
 * indices for a list of web documents containing the specified word.
 */

static void ProcessResponse(const char *word, hashset *stop_words, hashset *words_from_newses) {
  if (WordIsWellFormed(word)) {
    void *isStopWord = HashSetLookup(stop_words, &word);
    if (isStopWord != NULL) {
      printf("\tToo common a word to be taken seriously. Try something more specific.\n");
      return;
    }
    Word_Freqs_IN_Newses *freqs = HashSetLookup(words_from_newses, &word); 
    if (freqs == NULL) {
      printf("None of today's news articles contain the word \"%s\".\n", word);
      return;
    }
    vector *newses = freqs->freq_vec;
    VectorSort(newses, vector_compare_fn);
    for (int i = 0; i < VectorLength(newses) && i < 10; i++) {
      News *News = VectorNth(newses, i);
      char *times = "times";
      if (News->count == 1) times = "time";
      printf("%d.) \"%s\" [search term occurs %d %s]\n\"%s\"\n", i + 1, News->name, News->count, times, News->adress);
    }
  } else {
    printf( "\tWe won't be allowing words like \"%s\" into our set of indices.\n", word);
  }
}

/**
 * Predicate Function: WordIsWellFormed
 * ------------------------------------
 * Before we allow a word to be inserted into our map
 * of indices, we'd like to confirm that it's a good search term.
 * One could generalize this function to allow different criteria, but
 * this version hard codes the requirement that a word begin with
 * a letter of the alphabet and that all letters are either letters, numbers,
 * or the '-' character.
 */

static bool WordIsWellFormed(const char *word) {
  int i;
  if (strlen(word) == 0)
    return true;
  if (!isalpha((int)word[0]))
    return false;
  for (i = 1; i < strlen(word); i++)
    if (!isalnum((int)word[i]) && (word[i] != '-'))
      return false;

  return true;
}
