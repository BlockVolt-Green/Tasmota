## Commands

1. Go to Mqtt and connect to testnet-mqtt.rddl.io, 1885, rddl-tasmota, bE91dLR49FmsTtR2xbFCJfmgaGwTqeZJ
2. Go to console and execute 
    - ```Mnemonic``` -> generate new seed and save it for future use
    - ```PublicKeys``` -> Get all keys
    - ```PlanetmintAPI https://testnet-api.rddl.io```
    - ```PlanetmintDenom plmnt```
    - ```PlanetmintChainID planetmint-testnet-1```
    - ```NotarizationPeriodicity 3600```
    - ```NexusAPI https://nexus-ts.onrender.com/api/notarize/```
    - ```NexusAuthToken 31827105-6a82-4474-8c59-ef7cea8a100d```
    - ```Meter elite``` -> if you want to connect elite 440
    - ```Meter default``` -> If you want default tasmota autoconfig
3. Register machine, make post request on ```https://testnet-ta.rddl.io/register/<Machine CID>``` 
4. Fund machine, go to ```https://testnet-faucet.rddl.io/``` and put your address to get 100 plmt tokens
5  ```Balance``` check if balance is reflecting in your console 
6. Run ```AttestMachine Elite Blockvolt <Machine CID>```

