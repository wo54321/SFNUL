/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <SFNUL.hpp>

int main() {
	// Resolve our hostname to an address.
	auto addresses = sfn::IpAddress::Resolve( "www.ietf.org" );

	// Check if the name resolution was unsuccessful.
	if( addresses.empty() ) {
		std::cout << "Could not resolve hostname \"www.ietf.org\" to an address.\n";
		return 1;
	}

	// Create our TLS connection over a TCP transport.
	// If you want to skip authentication of the remote host
	// specify sfn::TlsVerificationType::OPTIONAL.
	auto connection = sfn::TlsConnection<sfn::TcpSocket, sfn::TlsEndpointType::CLIENT, sfn::TlsVerificationType::REQUIRED>::Create();

	// Set the Common Name we expect in the peer certificate.
	connection->SetPeerCommonName( "*.ietf.org" );

	// Our trusted CA certificate in Base64 DER format.
	std::string trusted_certificate{
		"-----BEGIN CERTIFICATE-----\r\n"
		"MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\r\n"
		"MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\r\n"
		"U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\r\n"
		"NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\r\n"
		"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\r\n"
		"ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\r\n"
		"DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\r\n"
		"8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\r\n"
		"+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\r\n"
		"X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\r\n"
		"K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\r\n"
		"1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\r\n"
		"A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\r\n"
		"zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\r\n"
		"YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\r\n"
		"bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\r\n"
		"DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\r\n"
		"L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\r\n"
		"eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\r\n"
		"xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\r\n"
		"VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\r\n"
		"WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\r\n"
		"-----END CERTIFICATE-----\r\n"
	};

	// Add our trusted certificate to the store to verify against.
	connection->AddTrustedCertificate( sfn::TlsCertificate::Create( trusted_certificate ) );

	// Connect the to the TLS endpoint.
	connection->Connect( sfn::Endpoint{ addresses.front(), 443 } );

	// Construct our HTTP request.
	std::string request( "GET / HTTP/1.0\r\nHost: www.ietf.org\r\n\r\n" );

	// Send our HTTP request.
	connection->Send( request.c_str(), request.size() );

	// Start a network processing thread.
	sfn::Start();

	// Keep waiting until the remote has signalled that it has nothing more to send.
	while( !connection->RemoteHasShutdown() ) {
	}

	std::array<char, 1024> reply;
	std::size_t reply_size = 0;

	do {
		// Dequeue any data we receive from the remote host.
		reply_size = connection->Receive( reply.data(), reply.size() );

		// Print out the data.
		for( std::size_t index = 0; index < reply_size; index++ ) {
			std::cout << reply[index];
		}
	} while( reply_size );

	// Shutdown our side.
	connection->Shutdown();

	// Wait for the transport to send the last goodbye messages.
	while( connection->BytesToSend() ) {
	}

	// Close the connection.
	connection->Close();

	// Stop all network processing threads.
	sfn::Stop();

	return 0;
}
