#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libplatform/libplatform.h>
#include <uv.h>
#include "v8.h"

#include "./helpers/strings.hpp"
#include "./helpers/file-manager.hpp"
#include "./custom-functions/print.hpp"
#include "./custom-functions/timer.hpp"
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
