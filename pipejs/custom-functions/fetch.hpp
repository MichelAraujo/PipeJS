
void Fetch(const v8::FunctionCallbackInfo<v8::Value> &args) {
  // Get V8 string from argument and convert into a std string
  v8::Isolate* isolate = args.GetIsolate();
  v8::String::Utf8Value v8String(isolate, args[0]);
  std::string urlFromArgs(*v8String);

  printf("%s FETCH file - \n", urlFromArgs.c_str());

  // Mount URL object to fulfil the split (domain, port, path)
  URL url = MountURL(urlFromArgs);

  SSLHandler ssl;
  ssl.init();

  TCPHandler tcp;
  tcp.connect(DEFAULT_LOOP, &ssl, url);
}