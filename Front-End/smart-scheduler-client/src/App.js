import React, { useState, useEffect } from 'react';
import axios from 'axios';

// THAY ĐỔI Ở ĐÂY: Dán link Render của bạn vào đây (nhớ bỏ dấu / ở cuối)
const API_BASE_URL = "https://arcsystem-backend.onrender.com"; 

function App() {
  const [timers, setTimers] = useState([]);
  const [title, setTitle] = useState('');
  const [dateTime, setDateTime] = useState('');

  const fetchTimers = async () => {
    try {
      // SỬA: Dùng API_BASE_URL
      const res = await axios.get(`${API_BASE_URL}/api/timers/next`);
      setTimers(res.data);
    } catch (err) {
      console.error("Lỗi lấy dữ liệu:", err);
    }
  };

  useEffect(() => {
    fetchTimers();
  }, []);

  const addTimer = async (e) => {
    e.preventDefault();
    const timestamp = Math.floor(new Date(dateTime).getTime() / 1000);
    
    try {
      // SỬA: Dùng API_BASE_URL
      await axios.post(`${API_BASE_URL}/api/timers`, {
        title: title,
        trigger_at: timestamp
      });
      setTitle('');
      fetchTimers();
    } catch (err) {
      alert("Lỗi khi lưu dữ liệu!");
    }
  };

  // ... (Phần return bên dưới giữ nguyên)

  return (
    <div style={{ padding: '20px', fontFamily: 'Arial' }}>
      <h1>⏱️ RabbitTank Scheduler</h1>
      
      {/* Form thêm mới */}
      <form onSubmit={addTimer} style={{ marginBottom: '30px' }}>
        <input 
          type="text" placeholder="Tiêu đề..." value={title}
          onChange={(e) => setTitle(e.target.value)} required 
        />
        <input 
          type="datetime-local" value={dateTime}
          onChange={(e) => setDateTime(e.target.value)} required 
        />
        <button type="submit">Thêm Timer</button>
      </form>

      {/* Danh sách hiển thị */}
      <h2>3 Task sắp tới (Cho ESP32):</h2>
      <ul>
        {timers.map(t => (
          <li key={t._id}>
            <b>{t.title}</b> - {new Date(t.trigger_at * 1000).toLocaleString()} 
            <br/><small>Unix: {t.trigger_at}</small>
          </li>
        ))}
      </ul>
    </div>
  );
}

export default App;