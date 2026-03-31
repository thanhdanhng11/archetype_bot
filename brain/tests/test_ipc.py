import pytest
import json
import sys
import os
from unittest.mock import patch, MagicMock

# Resolve paths
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../')))

from brain.ipc_sender import SignalTransmitter

class TestSignalTransmitter:
    @patch('brain.ipc_sender.redis.Redis')
    def test_fire_kill_signal(self, mock_redis):
        # 1. Setup Mock Redis Client
        mock_client = MagicMock()
        mock_redis.return_value = mock_client
        
        # 2. Initialize the IPC Transmitter
        transmitter = SignalTransmitter()
        
        # 3. Fire the simulated signal
        transmitter.fire_kill_signal(
            target_user="0xTArGEt",      # Testing lowercase enforcement
            debt_asset="0xDeBT",
            collateral_asset="0xCoL",
            amount_to_repay=999999999999 # Testing large integer to string bounds
        )
        
        # 4. Assert LPUSH was called exactly once to the Redis DB
        mock_client.lpush.assert_called_once()
        
        # 5. Verify the Arguments Passed across the boundary
        args, kwargs = mock_client.lpush.call_args
        queue_name = args[0]
        raw_payload = args[1]
        payload = json.loads(raw_payload)
        
        assert queue_name == "liquidation_orders"
        assert payload["target"] == "0xtarget"       # Must be lowercased
        assert payload["asset"] == "0xdebt"          # Must be lowercased
        assert payload["collateral"] == "0xcol"      # Must be lowercased
        assert payload["amount"] == "999999999999"   # Must be perfectly casted string
        assert "timestamp" in payload
