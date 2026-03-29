require('dotenv').config(); // Phải nằm ở trên cùng
const express = require('express');
// ...

const mongoose = require('mongoose');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.json());

// Kết nối MongoDB
const mongoURI = process.env.MONGO_URI; 
const PORT = process.env.PORT || 5000;

mongoose.connect(mongoURI)
  .then(() => console.log("✅ Đã kết nối RabbiTankCluster thành công!"))
  .catch(err => console.log("❌ Lỗi kết nối:", err));

// Định nghĩa cấu trúc Timer
const timerSchema = new mongoose.Schema({
  title: String,
  trigger_at: Number, // Unix Timestamp (giây)
  status: { type: String, default: 'pending' }
});

const Timer = mongoose.model('Timer', timerSchema);

/**
 * Hàm hỗ trợ: Dọn dẹp dữ liệu cũ
 * Xóa các Timer có trigger_at đã trôi qua quá 3 ngày (3 * 24 * 3600 giây)
 */
const cleanupOldTimers = async () => {
  const threeDaysInSeconds = 3 * 24 * 60 * 60;
  const now = Math.floor(Date.now() / 1000);
  const threshold = now - threeDaysInSeconds;

  try {
    const result = await Timer.deleteMany({ trigger_at: { $lt: threshold } });
    if (result.deletedCount > 0) {
      console.log(`🧹 Đã dọn dẹp ${result.deletedCount} bản ghi cũ hơn 3 ngày.`);
    }
  } catch (err) {
    console.error("❌ Lỗi khi dọn dẹp dữ liệu:", err);
  }
};

// API 1: Lấy 3 Timer gần nhất cho ESP32 và React
app.get('/api/timers/next', async (req, res) => {
  // Mỗi khi có ai đó hỏi lấy dữ liệu, ta tranh thủ dọn dẹp DB luôn
  await cleanupOldTimers();

  const now = Math.floor(Date.now() / 1000);
  
  try {
    const nextTimers = await Timer.find({ 
      trigger_at: { $gt: now },
      status: 'pending' 
    })
    .sort({ trigger_at: 1 })
    .limit(3);
    
    res.json(nextTimers);
  } catch (err) {
    res.status(500).json({ error: "Lỗi truy vấn dữ liệu" });
  }
});

// API 2: Thêm Timer mới từ React
app.post('/api/timers', async (req, res) => {
  try {
    const newTimer = new Timer(req.body);
    await newTimer.save();
    res.json({ message: "Đã lưu Timer mới!", data: newTimer });
  } catch (err) {
    res.status(500).json({ error: "Lỗi khi lưu dữ liệu" });
  }
});

// Render cần sử dụng port mà nó cấp phát, không phải lúc nào cũng là 5000
app.listen(PORT, () => console.log(`🚀 Server đang chạy tại port ${PORT}`));