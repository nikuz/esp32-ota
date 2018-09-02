#ifndef EspOta_h
#define EspOta_h

#include "Arduino.h"

class EspOta {
  public:
    EspOta(
      const String host,
      const int port,
      const String bin,
      const char *pKey
    );
    ~EspOta();

    void begin();

  private:
    void getCurrentETag();
    void validate();
    void update();
    void end();
};

#endif

