
## :: set wasm code and abi

curl -u wiccuser:123456 -d '{"jsonrpc":"2.0","id":"curltext","method":"setcodewasmcontracttx","params":["wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4","0-1","/home/xiaoyu/src/token.wasm","/home/xiaoyu/src/token.abi",11000000000000,"token"]}' -H 'content-type:application/json;' http://127.0.0.1:6968

You can get the token.wasm and token.abi in wasm.cdt/samples

## :: create token

curl -u wiccuser:123456 -d '{"jsonrpc":"2.0","id":"curltext","method":"callwasmcontracttx","params":["wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4","0-1","create",{"issuer":"walker","maximum_supply":"100000.0000 BTC"},1000000,11000000000000]}' -H 'content-type:application/json;' http://127.0.0.1:6968

## :: issue 

curl -u wiccuser:123456 -d '{"jsonrpc":"2.0","id":"curltext","method":"callwasmcontracttx","params":["wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4","0-1","issue",{"to":"xiaoyu","quantity":"800.0000 BTC","memo":"issue to xiaoyu"},1000000,11000000000000]}' -H 'content-type:application/json;' http://127.0.0.1:6968

## :: transfer

curl -u wiccuser:123456 -d '{"jsonrpc":"2.0","id":"curltext","method":"callwasmcontracttx","params":["wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4","0-1","transfer",{"from":"xiaoyu","to":"walker","quantity":"100.0000 BTC","memo":"transfer to walker"},1000000,11000000000000]}' -H 'content-type:application/json;' http://127.0.0.1:6968



