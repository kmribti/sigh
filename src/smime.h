/*! @file smime.h
 *
 * @brief Handle S/MIME messages
 *
 * @author Christian Roessner <c@roessner.co>
 * @version 1607.1.4
 * @date 2016-06-10
 * @copyright Copyright 2016 Christian Roessner <c@roessner.co>
 */

#ifndef SRC_SMIME_H_
#define SRC_SMIME_H_

#include <libmilter/mfapi.h>
#include <openssl/pem.h>

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <boost/algorithm/string.hpp>

void init_openssl(void);
void deinit_openssl(void);

namespace smime {
    using boost::split;
    using boost::trim;
    using boost::is_any_of;
    using boost::token_compress_on;

    using split_t =  std::vector<std::string>;

    // Wrapper functions
    void bioDeleter(BIO *);
    void x509Deleter(X509 *);
    void x509InfoDeleter(X509_INFO *);
    void evpPkeyDeleter(EVP_PKEY *);
    void pkcs7Deleter(PKCS7 *);
    void stackOfX509Deleter(STACK_OF(X509) *);
    void stackOfX509InfoDeleter(STACK_OF(X509_INFO) *);

    // Type definitions for OpenSSL
    using BIO_ptr = std::unique_ptr<BIO, decltype(&bioDeleter)>;
    using X509_ptr = std::unique_ptr<X509, decltype(&x509Deleter)>;
    using X509_INFO_ptr = std::unique_ptr<X509_INFO,
            decltype(&x509InfoDeleter)>;
    using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&evpPkeyDeleter)>;
    using PKCS7_ptr = std::unique_ptr<PKCS7, decltype(&pkcs7Deleter)>;
    using STACK_OF_X509_ptr = std::unique_ptr<STACK_OF(X509),
            decltype(&stackOfX509Deleter)>;
    using STACK_OF_X509_INFO_ptr = std::unique_ptr<STACK_OF(X509_INFO),
            decltype(&stackOfX509InfoDeleter)>;

    /*!
     * @brief S/MIME handling
     *
     * This class creates a S/MIME signed mail if possible and directly talks
     * to the milter to add, modify headers and finally replace the body.
     */
    class Smime {
    public:
        /*!
         * @brief Constructor
         */
        Smime(SMFICTX *);

        /*!
         * @brief Destructor
         */
        ~Smime(void) = default;

        inline bool isSmimeSigned(void) const { return smimeSigned; }

        /*!
         * @brief Sign a mail
         */
        void sign(void);

    private:
        /*!
         * @brief Add headers that were generated in sign()
         *
         * When a message was signed, new headers need to be added to the
         * message.
         */
        int addHeader(const std::string &, const std::string &);

        /*!
         * @brief Remove headers from original mail
         *
         * When signing, new headers are generated and existing ones are
         * embedded inside the new message body.
         */
        int removeHeader(const std::string &);

        /*!
         * @brief Error handler for S/MIME signing problems
         *
         * This method is always called, if some signing operations failed.
         * It also sets the genericError flag for the connected client.
         */
        void handleSSLError(void);

        /*!
         * @brief Load intermediate S/MIME certificates
         *
         * The S/MIME certificate may have several intermediate certficates
         * concatenated. Try to load them for signing.
         *
         * As this function uses a jump label, the code is separated from the
         * main signing routine sign().
         */
        STACK_OF_X509_ptr loadIntermediate(const std::string &);

        /*!
         * @brief The current client context that was created on connect
         *
         * This class works directly on the original message.
         */
        SMFICTX *ctx;

        /*!
         * @brief Flag that indicates, if signing was possible
         *
         * If a certificate and key was provided and signing was successful,
         * this flag is used in mlfi_eom to evaluate a reply message.
         */
        bool smimeSigned;

        /*!
         * @brief A normalized version of the MAIL FROM address
         *
         * We strip away '<' and '>' to easily lookup required information in
         * our cert store, which is provided by the map class.
         */
        std::string mailFrom;
    };

}  // namespace smime

#endif  // SRC_SMIME_H_
