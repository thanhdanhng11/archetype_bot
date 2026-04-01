module.exports = {
  apps: [
    {
      name: "archetype_db_client",
      script: "./run_db_client.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '500M'
    },
    {
      name: "archetype_ipc",
      script: "./run_ipc.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '500M'
    },
    {
      name: "archetype_listener",
      script: "./run_listener.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '1G'
    },
    {
      name: "archetype_main",
      script: "./run_main.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '1G'
    },
    {
      name: "archetype_hunter",
      script: "./run_hunter.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '1G'
    },
    {
      name: "archetype_decoder",
      script: "./run_decoder.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '1G'
    },
    {
      name: "archetype_simulator",
      script: "./run_simulator.sh",
      interpreter: "bash",
      autorestart: true,
      watch: false,
      max_memory_restart: '1G'
    }
  ]
};
