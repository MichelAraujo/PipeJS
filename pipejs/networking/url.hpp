using namespace std;

struct URL {
  string protocol;
  string domain;
  string path;
};

URL MountURL(string urlSring) {
  URL url;
  int stringStart;
  int size = urlSring.size();
  bool isDomain = false;

  for (int index = 0; index < size; index++) {
    if (urlSring[index] == ':') {
      printf("protocol \n");
      url.protocol = urlSring.substr(0, index);
      index = index + 3; // Number 3 is because "://" in the full string
      stringStart = index;
    }

    if ((urlSring[index] == '/' && isDomain == false) || (isDomain == false)) {
      printf("start %d \n", stringStart);
      printf("index %d \n", index);

      // url.domain = urlSring.substr(stringStart, index - stringStart);
      stringStart = index;
      isDomain = true;
      index = size + 1; // Just to stop the loop
    }
  }

  // url.path = urlSring.substr(stringStart, size - stringStart);
  url.path = "/";
  
  return url;
}
