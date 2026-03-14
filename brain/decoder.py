import json 
import os
from web3 import Web3

class EventDecoder: 
    def __init__(self):
        """
        Initializes the decoder and loads the tactical ABIs.
        We now track both basic tokens and complex lending pools.
        """
        base_dir = os.path.dirname(os.path.abspath(__file__))
        
        # Load ERC20 ABI
        with open(os.path.join(base_dir, 'abis', 'erc20.json'), 'r') as file:
            self.erc20_abi = json.load(file)
            
        # Load Aave V3 Pool ABI
        with open(os.path.join(base_dir, 'abis', 'aave_pool.json'), 'r') as file:
            self.aave_abi = json.load(file)
            
        self.w3 = Web3()
        
        # Contract factories for decoding
        self.erc20_contract = self.w3.eth.contract(abi=self.erc20_abi)
        self.aave_contract = self.w3.eth.contract(abi=self.aave_abi)

    def decode_transfer(self, raw_log):
        try:
            decoded = self.erc20_contract.events.Transfer().process_log(raw_log)
            return {
                "type": "TRANSFER",
                "contract": decoded['address'].lower(),
                "from": decoded['args']['from'].lower(),
                "to": decoded['args']['to'].lower(),
                "value": decoded['args']['value']
            }
        except Exception:
            return None

    def decode_aave_borrow(self, raw_log):
        """
        Extracts debt creation events. This is the first step in finding liquidation targets.
        """
        try:
            decoded = self.aave_contract.events.Borrow().process_log(raw_log)
            return {
                "type": "BORROW",
                "protocol": "AAVE_V3",
                "pool": decoded['address'].lower(),
                "asset": decoded['args']['reserve'].lower(),
                "user": decoded['args']['user'].lower(),
                "amount": decoded['args']['amount']
            }
        except Exception:
            return None

    def decode_aave_repay(self, raw_log):
        """
        Extracts debt reduction events. We must track this to ensure our targets 
        haven't already saved themselves from liquidation.
        """
        try:
            decoded = self.aave_contract.events.Repay().process_log(raw_log)
            return {
                "type": "REPAY",
                "protocol": "AAVE_V3",
                "pool": decoded['address'].lower(),
                "asset": decoded['args']['reserve'].lower(),
                "user": decoded['args']['user'].lower(),
                "amount": decoded['args']['amount']
            }
        except Exception:
            return None

if __name__ == "__main__":
    print("[*] EventDecoder module upgraded with Lending Protocol intelligence.")
