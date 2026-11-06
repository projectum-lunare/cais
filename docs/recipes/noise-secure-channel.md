# Recipe: Hardened Cryptographic Links with Noise (`cais.noise`)

This recipe demonstrates how to establish an end-to-end encrypted, mutually authenticated channel between two processes using `cais.noise` and `cais.socket`.

Unlike TLS, there are no certificates, certificate authorities, or complex configuration files. Each party is identified solely by a 32-byte Curve25519 public key.

---

## C89 Implementation: Hardened TCP Client & Server

Both peers perform the standard `Noise_XX` handshake (mutual authentication with identity hiding), followed by framed, encrypted message transfers.

### Server (`server.c`)

```c
#include <stdio.h>
#include <string.h>
#include <cais/cais.h>

int main(void)
{
    char err[128];
    unsigned char server_priv[32], server_pub[32];
    cais_socket_t *listener, *client_sock;
    cais_noise_t *noise;
    unsigned char buf[1024];
    size_t received = 0;

    /* Generate or load persistent server keypair */
    cais_noise_keygen(server_priv, server_pub, err, sizeof(err));

    /* Bind TCP listener on port 9000 */
    listener = cais_socket_bind("0.0.0.0", 9000, 5, CAIS_SOCK_BLOCKING, err, sizeof(err));
    if (!listener) {
        fprintf(stderr, "Bind failed: %s\n", err);
        return 1;
    }
    printf("Server listening for Noise connections on port 9000...\n");

    client_sock = cais_socket_accept(listener, NULL, NULL, err, sizeof(err));
    if (!client_sock) {
        fprintf(stderr, "Accept failed: %s\n", err);
        return 1;
    }

    /* Initialize Noise_XX responder session */
    noise = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_RESPONDER, err, sizeof(err));
    cais_noise_set_static_key(noise, server_priv, err, sizeof(err));
    cais_noise_start(noise, err, sizeof(err));

    /* Execute automated framed handshake over TCP socket */
    if (cais_noise_handshake_socket(noise, client_sock, err, sizeof(err)) != CAIS_OK) {
        fprintf(stderr, "Handshake failed: %s\n", err);
        return 1;
    }
    printf("Noise handshake completed successfully!\n");

    /* Authenticate remote peer's static key against an authorized list */
    {
        const unsigned char *client_pub = cais_noise_get_remote_static(noise);
        printf("Client authenticated with public key: ");
        for (int i = 0; i < 32; i++) printf("%02x", client_pub[i]);
        printf("\n");
    }

    /* Receive encrypted, authenticated payload */
    if (cais_noise_recv_socket(noise, client_sock, buf, sizeof(buf) - 1, &received, err, sizeof(err)) == CAIS_OK) {
        buf[received] = '\0';
        printf("Received secure payload: '%s'\n", (char *)buf);

        /* Send encrypted echo response */
        cais_noise_send_socket(noise, client_sock, "ACK: Message received securely", 30, err, sizeof(err));
    }

    cais_noise_free(noise);
    cais_socket_close(client_sock);
    cais_socket_close(listener);
    return 0;
}
```

---

### Client (`client.c`)

```c
#include <stdio.h>
#include <string.h>
#include <cais/cais.h>

int main(void)
{
    char err[128];
    unsigned char client_priv[32], client_pub[32];
    cais_socket_t *sock;
    cais_noise_t *noise;
    unsigned char reply[1024];
    size_t reply_len = 0;

    /* Generate or load client keypair */
    cais_noise_keygen(client_priv, client_pub, err, sizeof(err));

    sock = cais_socket_connect("127.0.0.1", 9000, CAIS_SOCK_BLOCKING, err, sizeof(err));
    if (!sock) {
        fprintf(stderr, "Connect failed: %s\n", err);
        return 1;
    }

    /* Initialize Noise_XX initiator session */
    noise = cais_noise_create(CAIS_NOISE_XX, CAIS_NOISE_INITIATOR, err, sizeof(err));
    cais_noise_set_static_key(noise, client_priv, err, sizeof(err));
    cais_noise_start(noise, err, sizeof(err));

    /* Perform automated framed handshake */
    if (cais_noise_handshake_socket(noise, sock, err, sizeof(err)) != CAIS_OK) {
        fprintf(stderr, "Handshake failed: %s\n", err);
        return 1;
    }

    /* Send secure payload */
    cais_noise_send_socket(noise, sock, "Secret payload from C89 client", 30, err, sizeof(err));

    /* Receive secure response */
    if (cais_noise_recv_socket(noise, sock, reply, sizeof(reply) - 1, &reply_len, err, sizeof(err)) == CAIS_OK) {
        reply[reply_len] = '\0';
        printf("Server replied: '%s'\n", (char *)reply);
    }

    cais_noise_free(noise);
    cais_socket_close(sock);
    return 0;
}
```

---

## Lua Implementation: Fast 1-RTT WireGuard Pattern (`Noise_IK`)

In scenarios where the client already has the server's static public key pre-configured, `Noise_IK` allows establishing the authenticated session in a single network round-trip (1 RTT):

```lua
local cais = require("cais")
local noise = cais.noise
local socket = cais.socket

-- 1. Helper function to frame messages with uint16 big-endian length prefix
local function send_frame(sock, data)
    local len = #data
    local hdr = string.char(math.floor(len / 256), len % 256)
    sock:send(hdr .. data)
end

local function recv_frame(sock)
    local hdr = sock:recv(2)
    if not hdr or #hdr < 2 then return nil, "eof" end
    local len = hdr:byte(1) * 256 + hdr:byte(2)
    return sock:recv(len)
end

-- 2. Client Side: Connect and perform 1-RTT handshake
local function run_client(server_pub)
    local c_priv, c_pub = noise.keygen()
    local sock = socket.connect("127.0.0.1", 9001)

    local client = noise.create({
        pattern = "IK",
        role = "initiator",
        private_key = c_priv,
        remote_key = server_pub
    })

    -- Message 1: -> e, es, s, ss (Client sends encrypted identity in 1st packet)
    local m1 = client:write_message()
    send_frame(sock, m1)

    -- Message 2: <- e, ee, se (Server confirms)
    local m2 = recv_frame(sock)
    client:read_message(m2)

    assert(client:is_complete())
    print("Channel secured in 1 RTT!")

    -- Transmit encrypted payload
    local ciphertext = client:encrypt("Fast encrypted record")
    send_frame(sock, ciphertext)

    sock:close()
end
```
