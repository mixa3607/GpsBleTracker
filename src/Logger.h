#include <Stream.h>

class Logger
{
private:
  const char * _prefix;
  Stream *_output;

public:
  Logger(Stream *const output, const char *const prefix)
  {
    _output = output;
    _prefix = prefix;
  }

  void Log(String text)
  {
    if (_output)
    {
      _output->println(String(_prefix) + ": " + text);
    }
  }
};