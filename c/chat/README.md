# chat

A chatroom server for multiple peers over TCP and HTTP ports.

| Argmuent | Pair value (if any) | Meaning | Required |
|----------|---------------------|---------|----------|
| `s`/`server`   | *none*        | Starta the program in the server mode. | No |
| `c`/`client`   | *none*        | Starts the program in the client mode. | No |
| `--net` | `eth`/`slip`         | Sets the networking driver (serial SLIP or Ethernet) | No, default is SLIP. |
| `--ip`  | IPv4 address | Sets the peer's IPv4 address. | Yes, in client mode |

## Example

```sh
bg CHAT s eth

bg CHAT client --ip 10.3.4.1 --net eth
```