
-- bảng trạng thái đồng bộ. bảng này cho biết bot python đang đọc tới block nào để trong trường hợp bị crash thì có thể tự động khôi phục
CREATE TABLE sync_state (
    network_id VARCHAR(20) PRIMARY KEY,
    last_processed_block BIGINT NOT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
); 

-- bảng danh mục token. lưu trữ thông tin hằng số để tránh việc phải gọi on-chain hỏi số thập phân liên tục
CREATE TABLE tokens (
    token_address CHAR(42) PRIMARY KEY,
    symbol  varchar(15) NOT NULL,
    decimals SMALLINT NOT NULL 
);

-- bảng vị thế người vay (lending-positions). bảng này lưu trữ trạng thái nợ cục bộ từng loại tài sản
CREATE TABLE lending_positions (
    user_address CHAR(42) NOT NULL, 
    protocol VARCHAR(20) NOT NULL,
    asset CHAR(42) NOT NULL,

    debt_amount NUMERIC(78, 0) DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (user_address, protocol, asset)
);

CREATE INDEX idx_user_address ON lending_positions (user_address);