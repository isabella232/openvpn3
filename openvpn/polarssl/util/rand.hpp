//    OpenVPN -- An application to securely tunnel IP networks
//               over a single port, with support for SSL/TLS-based
//               session authentication and key exchange,
//               packet encryption, packet authentication, and
//               packet compression.
//
//    Copyright (C) 2012-2016 OpenVPN Technologies, Inc.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License Version 3
//    as published by the Free Software Foundation.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program in the COPYING file.
//    If not, see <http://www.gnu.org/licenses/>.

// Wrap the mbed TLS Cryptographic Random API defined in <mbedtls/ctr_drbg.h>
// so that it can be used as the primary source of cryptographic entropy by
// the OpenVPN core.

#ifndef OPENVPN_MBEDTLS_UTIL_RAND_H
#define OPENVPN_MBEDTLS_UTIL_RAND_H

#include <mbedtls/entropy_poll.h>
#include <mbedtls/ctr_drbg.h>

#include <openvpn/random/randapi.hpp>

namespace openvpn {

  class PolarSSLRandom : public RandomAPI
  {
  public:
    OPENVPN_EXCEPTION(rand_error_polarssl);

    typedef RCPtr<PolarSSLRandom> Ptr;

    PolarSSLRandom(const bool prng)
    {
      if (mbedtls_ctr_drbg_seed(&ctx, entropy_poll, nullptr, nullptr, 0) < 0)
	throw rand_error_polarssl("CTR_DRBG init");

      // If prng is set, configure for higher performance
      // by reseeding less frequently.
      if (prng)
	mbedtls_ctr_drbg_set_reseed_interval(&ctx, 1000000);
    }

    // Random algorithm name
    virtual std::string name() const
    {
      return "mbedTLS-CTR_DRBG";
    }

    // Return true if algorithm is crypto-strength
    virtual bool is_crypto() const
    {
      return true;
    }

    // Fill buffer with random bytes
    virtual void rand_bytes(unsigned char *buf, size_t size)
    {
      if (!rndbytes(buf, size))
	throw rand_error_polarssl("CTR_DRBG rand_bytes");
    }

    // Like rand_bytes, but don't throw exception.
    // Return true on successs, false on fail.
    virtual bool rand_bytes_noexcept(unsigned char *buf, size_t size)
    {
      return rndbytes(buf, size);
    }

  private:
    bool rndbytes(unsigned char *buf, size_t size)
    {
      return mbedtls_ctr_drbg_random(&ctx, buf, size) < 0 ? false : true;
    }

    static int entropy_poll(void *data, unsigned char *output, size_t len)
    {
      size_t olen;
      return mbedtls_platform_entropy_poll(data, output, len, &olen);
    }

    mbedtls_ctr_drbg_context ctx;
  };

}

#endif
