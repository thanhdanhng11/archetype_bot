import os
import asyncpg
from dotenv import load_dotenv

# Load configuration from the .env file
load_dotenv()

DB_USER = os.getenv("DB_USER", "postgres")
DB_PASSWORD = os.getenv("DB_PASSWORD", "postgres")
DB_HOST = os.getenv("DB_HOST", "localhost")
DB_PORT = os.getenv("DB_PORT", "54829")
DB_NAME = os.getenv("DB_NAME", "archetype_db")

class DatabaseClient: 
    def __init__(self): 
        """
        Initializes the databaseclient without an active connection. the connection pool must be established asynchronously via connect()
        """
        self.pool = None 

    async def connect(self): 
        """
        Establishes an asynchronous connection pool to PostgreSQL.
        A pool is mandatory to handle high-throughout, concurrent database operations without the overhead of opening new
        """
        dsn = f"postgresql://{DB_USER}:{DB_PASSWORD}@{DB_HOST}:{DB_PORT}/{DB_NAME}"
        try:
            # We initialize a minimalist pool. min_size=1 ensures it stays alive, max_size=10 prevents overwhelming the database server.
            self.pool = await asyncpg.create_pool(dsn=dsn, min_size=1, max_size=10)
            print("Database connection pool established successfully.")
        except Exception as e:
            print(f"Critical Error: Failed to connect to the database -> {e}")

    async def close(self): 
        """
        Gracefully closes the connection pool, releasing all underlying database connections.
        """
        if self.pool:
            await self.pool.close()
            print("Database connection pool closed safely.")

    async def update_sync_state(self, network_id: str, block_number:int): 
        """
        Update the latest proceesed block number into the 'sync_state' table. This acts as the system's save point. If the server crashes, the bot knows exactly which block to resume from.
        """ 
        query = """
            INSERT INTO sync_state (network_id, last_processed_block) 
            VALUES ($1, $2) 
            ON CONFLICT (network_id) 
            DO UPDATE SET
                last_processed_block = EXCLUDED.last_processed_block, 
                updated_at = CURRENT_TIMESTAMP;
        """ 
        # Acquire a single connection from the pool, execute the query, and return it instantly 
        if self.pool: 
            async with self.pool.acquire() as connection: 
                await connection.execute(query, network_id, block_number) 
    
    async def update_debt_position(self, user: str, protocol: str, asset: str, amount_delta: int):
        """
        Atomically updates a user's debt balance in the lending_positions table.
        - If they Borrow, amount_delta is positive (+).
        - If they Repay, amount_delta is negative (-).
        
        Using ON CONFLICT allows the database engine to handle the math safely, 
        even if thousands of events are processing concurrently.
        """
        query = """
            INSERT INTO lending_positions (user_address, protocol, asset, debt_amount)
            VALUES ($1, $2, $3, $4)
            ON CONFLICT (user_address, protocol, asset) 
            DO UPDATE SET 
                debt_amount = lending_positions.debt_amount + EXCLUDED.debt_amount,
                updated_at = CURRENT_TIMESTAMP;
        """
        if self.pool:
            try:
                # Acquire a connection and execute the mathematical upsert
                async with self.pool.acquire() as connection:
                    await connection.execute(query, user, protocol, asset, amount_delta)
            except Exception as e:
                print(f"[-] Database Error (update_debt_position): {e}")

    async def get_active_debtors(self):
        """
        Fetches a list of unique user addresses who currently have an active debt balance.
        These are the targets the Apex Hunter will scan for liquidation vulnerability.
        """
        query = "SELECT DISTINCT user_address FROM lending_positions WHERE debt_amount > 0;"
        if self.pool:
            try:
                async with self.pool.acquire() as connection:
                    records = await connection.fetch(query)
                    return [record['user_address'] for record in records]
            except Exception as e:
                print(f"[-] Database Error (get_active_debtors): {e}")
                return []
        return []

# Quick local test execution
async def main_test(): 
    db = DatabaseClient() 
    await db.connect() 
    await db.update_sync_state("eth", 12345678) 
    await db.close() 

if __name__ == "__main__": 
    import asyncio 
    print("Testing DatabaseClient module...")
    asyncio.run(main_test())