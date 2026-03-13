import json 
import os
from web3 import Web3

class EventDecoder: 
    def __init__(self):
        # Resolve absolute path to ensure it works regradless of where the script is executed from
        base_dir = os.path.dirname(os.path.abspath(__file__))
        erc20_abi_path = os.path.join(base_dir, 'abis', 'erc20.json')

        with open(erc20_abi_path, 'r') as file:
            self.erc20_abi = json.load(file)

        self.w3 = Web3()

        # Create a contract factory. We don't bind it to a specific address yet, allowing us to decode Transfer events from ANY ERC20 token contract.
        self.erc20_contract = self.w3.eth.contract(abi=self.erc20_abi)

    def decode_transfer(self, raw_log):
        try: 
            # The process_log finction automatically verifies if topic[0] matches the Transfer signature
            decoded_event = self.erc20_contract.events.Transfer().process_log(raw_log)
            
            # Extract the core arguments neatly
            args = decoded_event['args']
            return {
                "contract_address": decoded_event['address'].lower(),
                "from": args['from'].lower(),
                "to": args['to'].lower(),
                "value": args['value']
            }
        except Exception:
            # Silent discard: If the log is not a Transfer event, we ignore the noise.
            return None
        

if __name__ == "__main__":
    print("EventDecoder module is ready.")
