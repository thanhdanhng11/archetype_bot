#!/bin/bash
set -e

echo "🚀 Bắt đầu quá trình Cài đặt Môi trường VPS cho Archetype Bot..."

echo "📦 1. Cập nhật hệ thống và cài đặt Dependencies..."
sudo apt update && sudo apt upgrade -y
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs npm git docker.io docker-compose build-essential cmake gcc g++ python3 python3-pip python3-venv tmux libhiredis-dev nlohmann-json3-dev libfmt-dev libssl-dev libcurl4-openssl-dev
sudo npm install pm2 -g

echo "🐍 2. Thiết lập Môi trường Python (Brain)..."
python3 -m venv venv
source venv/bin/activate
pip install -r brain/requirements.txt

echo "🐳 3. Khởi động Cơ sở dữ liệu..."
sudo docker-compose up -d

echo "⚙️ 4. Biên dịch C++ Engine..."
cd engine
mkdir -p build && cd build
cmake ..
make -j$(nproc)
cd ../..

echo "✅ 5. Setup file môi trường (Nếu chưa có)..."
if [ ! -f .env ]; then
    cp .env.example .env
    echo "⚠️ Đã tạo file .env từ .env.example. Vui lòng mở ra và sửa lại Private Key!"
fi

echo "🌟 All Done! Môi trường C++ và Python đã sẵn sàng!"
echo "➡️  Để chạy bot 24/7, hãy dùng lệnh: pm2 start ecosystem.config.js"
