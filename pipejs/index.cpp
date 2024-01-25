#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <libgen.h>
#include <vector>
#include <iterator>
#include <algorithm>

#include <libplatform/libplatform.h>
#include <uv.h>
#include "v8.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include "./helpers/strings.hpp"
#include "./helpers/file-manager.hpp"
#include "./custom-functions/print.hpp"
#include "./custom-functions/timer.hpp"
#include "./custom-functions/fetch.hpp"
#include "./networking/SSLHandler.hpp"
#include "./networking/tcp.hpp"
#include "./pipejs.hpp"

int main(int argc, char* argv[]) {
  char *jsFile = argv[1];

  auto *pipejs = new PipeJS();
  std::unique_ptr<v8::Platform> platform =
    pipejs->initV8(argc, argv);
  
  pipejs->initVM();
  pipejs->initRuntime(jsFile);
  pipejs->Shutdown();

  return 0;
}
