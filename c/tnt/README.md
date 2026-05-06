# tnt

A TELNET server and remote shell.

| Argmuent | Pair value (if any) | Meaning | Required |
|----------|---------------------|---------|----------|
| `debug`   | *none*        | Starts in verbose mode. | No |
| `--net` | `eth`/`slip`         | Sets the networking driver (serial SLIP or Ethernet) | No, default is SLIP. |

## Example

```sh
bg TNT eth
```