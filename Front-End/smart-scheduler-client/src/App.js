import React, { useState, useEffect } from 'react';
import axios from 'axios';

function App() {
  const [timers, setTimers] = useState([]);
  const [title, setTitle] = useState('');
  const [dateTime, setDateTime] = useState('');

  // 1. Lấy danh sách 3 Timer gần nhất từ Server
  const fetchTimers = async () => {
    try {
      const res = await axios.get('http://localhost:5000/api/timers/next');
      setTimers(res.data);
    } catch (err) {
      console.error("Lỗi lấy dữ liệu:", err);
    }
  };

  useEffect(() => {
    fetchTimers();
  }, []);

  // 2. Hàm thêm Timer mới
  const addTimer = async (e) => {
    e.preventDefault();
    const timestamp = Math.floor(new Date(dateTime).getTime() / 1000);
    
    try {
      await axios.post('http://localhost:5000/api/timers', {
        title: title,
        trigger_at: timestamp
      });
      setTitle('');
      fetchTimers(); // Cập nhật lại danh sách ngay lập tức
    } catch (err) {
      alert("Lỗi khi lưu dữ liệu!");
    }
  };

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