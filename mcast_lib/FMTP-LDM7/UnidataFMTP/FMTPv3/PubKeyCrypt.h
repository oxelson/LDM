/**
 * Public key cryptography.
 *
 *        File: PubKeyCrypt.h
 *  Created on: Sep 2, 2020
 *      Author: Steven R. Emmerson
 */

#ifndef MCAST_LIB_FMTP_LDM7_UNIDATAFMTP_FMTPV3_PUB_KEY_CRYPT_H_
#define MCAST_LIB_FMTP_LDM7_UNIDATAFMTP_FMTPV3_PUB_KEY_CRYPT_H_

#include <cstdint>
#include <string>

#include <openssl/rsa.h>

#define USE_RSA 1

/**
 * Base class for public key cryptography.
 */
class PubKeyCrypt
{
protected:
#if USE_RSA
    RSA*              rsa;
    int               rsaSize;
    static const int  padding = RSA_PKCS1_OAEP_PADDING;
#endif

    /**
     * Default constructs.
     */
    PubKeyCrypt();

public:
    PubKeyCrypt(PubKeyCrypt& crypt) =delete;

    PubKeyCrypt(PubKeyCrypt&& crypt) =delete;

    /**
     * Destroys.
     */
    virtual ~PubKeyCrypt();

    PubKeyCrypt& operator=(const PubKeyCrypt& crypt) =delete;

    PubKeyCrypt& operator=(const PubKeyCrypt&& crypt) =delete;

    /**
     * Encrypts plaintext.
     *
     * @param[in]  plainText      Plain text
     * @param[out] cipherText     Encrypted text
     * @throw std::runtime_error  OpenSSL failure
     */
    virtual void encrypt(const std::string& plainText,
                 std::string&               cipherText) const =0;

    /**
     * Decrypts ciphertext.
     *
     * @param[in]  cipherText     Encrypted text
     * @param[out] plainText      Plain text
     * @throw std::runtime_error  OpenSSL failure
     */
    virtual void decrypt(const std::string& cipherText,
                 std::string&               plainText) const =0;
};

/**
 * A Public key.
 */
class PublicKey final : public PubKeyCrypt
{
public:
    /**
     * Constructs from a public-key certificate.
     *
     * @param[in] pubKey          X.509 public-key certificate in PEM format
     * @throw std::runtime_error  OpenSSL failure
     * @throw std::runtime_error  OpenSSL failure
     */
    PublicKey(const std::string& pubKey);

    /**
     * Encrypts plaintext using the public key.
     *
     * @param[in]  plainText      Plain text
     * @param[out] cipherText     Encrypted text
     * @throw std::runtime_error  OpenSSL failure
     */
    void encrypt(const std::string& plainText,
                 std::string&       cipherText) const override;

    /**
     * Decrypts ciphertext using the public key.
     *
     * @param[in]  cipherText     Encrypted text
     * @param[out] plainText      Plain text
     * @throw std::runtime_error  OpenSSL failure
     */
    void decrypt(const std::string& cipherText,
                 std::string&       plainText) const override;
};

/**
 * A public and private key-pair.
 */
class PrivateKey final : public PubKeyCrypt
{
    std::string pubKey; ///< Associated X.509 public key in PEM format

public:
    /**
     * Default constructs. A public/private key-pair is chosen at random.
     *
     * @throw std::runtime_error  OpenSSL failure
     */
    PrivateKey();

    /**
     * Returns the public key.
     *
     * @return  The X.509 public key in PEM format
     */
    const std::string& getPubKey() const noexcept;

    /**
     * Encrypts plaintext using the private key.
     *
     * @param[in]  plainText         Plain text
     * @param[in]  plainLen          Length of plain text in bytes
     * @param[out] cipherText        Encrypted text. Must not overlap plain
     *                               text.
     * @param[in]  cipherLen         Length of ciphertext buffer
     * @return                       Length of ciphertext in bytes
     * @throw std::invalid_argument  Ciphertext buffer is too small
     * @throw std::runtime_error     OpenSSL failure
     */
    int encrypt(const char* plainText,
                            const int   plainLen,
                            char*       cipherText,
                            int         cipherLen) const;

    /**
     * Encrypts plaintext using the private key.
     *
     * @param[in]  plainText      Plain text
     * @param[out] cipherText     Encrypted text. May be same object as plain
     *                            text.
     * @throw std::runtime_error  OpenSSL failure
     */
    void encrypt(const std::string& plainText,
                 std::string&       cipherText) const override;

    /**
     * Decrypts ciphertext using the public key.
     *
     * @param[in]  cipherText     Encrypted text
     * @param[out] plainText      Plain text
     * @throw std::runtime_error  OpenSSL failure
     */
    void decrypt(const std::string& cipherText,
                 std::string&       plainText) const override;
};

#endif /* MCAST_LIB_FMTP_LDM7_UNIDATAFMTP_FMTPV3_PUB_KEY_CRYPT_H_ */
