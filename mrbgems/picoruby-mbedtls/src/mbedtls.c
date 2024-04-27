#include <stdbool.h>
#include <mrubyc.h>

#include "cmac.h"
#include "cipher.h"
#include "digest.h"

void
mrbc_mbedtls_init(void)
{
  // mrbc_class *class_MbedTLS = mrbc_define_class(0, "MbedTLS", mrbc_class_object);

  gem_mbedtls_cmac_init();
  gem_mbedtls_cipher_init();
  gem_mbedtls_digest_init();
}

