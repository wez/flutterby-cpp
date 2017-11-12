#include "avr_autogen.h"
#include "flutterby/Progmem.h"
#include <string.h>
#include "flutterby/Debug.h"
#include "flutterby/Test.h"
#include "flutterby/Result.h"

using namespace flutterby;

Result<Unit, int> try_test() {
  auto intErr = Error(123);
  EXPECT(intErr.is_err());
  EXPECT(intErr.error() == 123);

  Try(intErr);

  return Ok<Unit, int>();
}

int main() {
  Result<bool, bool> res;

  EXPECT(res.is_empty());
  EXPECT(!res.is_ok());
  EXPECT(!res.is_err());

  res.set_ok(true);
  EXPECT(!res.is_empty());
  EXPECT(res.is_ok());
  EXPECT(!res.is_err());
  EXPECT(res.value());

  res.set_err(true);
  EXPECT(!res.is_empty());
  EXPECT(!res.is_ok());
  EXPECT(res.is_err());
  EXPECT(res.error());

  auto ok = Ok();
  EXPECT(ok.is_ok());

  auto err = Error();
  EXPECT(err.is_err());

  auto intOk = Ok(123);
  EXPECT(intOk.is_ok());
  EXPECT(intOk.value() == 123);

  EXPECT(try_test().is_err());
  return 0;
}
