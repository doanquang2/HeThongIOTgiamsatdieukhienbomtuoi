const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi" data-bs-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Doãn Quảng IoT - Control Panel</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css" rel="stylesheet">
    <style>
        :root {
            --glass-bg: rgba(30, 41, 59, 0.7);
            --glass-border: 1px solid rgba(255, 255, 255, 0.08);
            --primary-grad: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);
            --success-grad: linear-gradient(135deg, #10b981 0%, #059669 100%);
        }
        body {
            background-color: #0f172a;
            background-image: 
                radial-gradient(at 0% 0%, hsla(253,16%,7%,1) 0, transparent 50%), 
                radial-gradient(at 50% 0%, hsla(225,39%,30%,1) 0, transparent 50%), 
                radial-gradient(at 100% 0%, hsla(339,49%,30%,1) 0, transparent 50%);
            background-attachment: fixed;
            font-family: 'Segoe UI', system-ui, sans-serif;
            color: #e2e8f0;
            padding-bottom: 80px;
        }
        
        /* Navbar */
        .navbar {
            background: rgba(15, 23, 42, 0.85) !important;
            backdrop-filter: blur(12px);
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        .navbar-brand { font-weight: 700; letter-spacing: 0.5px; }
        
        /* Cards */
        .glass-card {
            background: var(--glass-bg);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border: var(--glass-border);
            border-radius: 20px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
            overflow: hidden;
        }
        .card-header {
            background: rgba(255,255,255,0.03);
            border-bottom: var(--glass-border);
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 1px;
            font-size: 0.85rem;
            color: #94a3b8;
            padding: 15px 20px;
        }
        
        /* Buttons & Toggles */
        .mode-switch .btn {
            border-radius: 50px;
            padding: 10px 30px;
            font-weight: 600;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .mode-switch .btn.active {
            background: var(--primary-grad);
            border: none;
            color: white;
            box-shadow: 0 4px 15px rgba(139, 92, 246, 0.4);
        }
        
        .pump-card {
            background: rgba(255,255,255,0.03);
            border: 1px solid rgba(255,255,255,0.05);
            border-radius: 16px;
            padding: 15px;
            cursor: pointer;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        .pump-card:hover {
            background: rgba(255,255,255,0.06);
            transform: translateY(-2px);
        }
        .pump-card.active {
            background: var(--success-grad);
            border-color: transparent;
            box-shadow: 0 8px 20px rgba(16, 185, 129, 0.3);
        }
        .pump-card .icon-box {
            width: 45px; height: 45px;
            border-radius: 12px;
            background: rgba(255,255,255,0.1);
            display: flex; align-items: center; justify-content: center;
            font-size: 1.2rem;
            color: rgba(255,255,255,0.7);
        }
        .pump-card.active .icon-box {
            background: rgba(255,255,255,0.25);
            color: white;
        }
        .pump-card .info { flex: 1; margin-left: 15px; }
        .pump-card .name { font-weight: 600; display: block; font-size: 1rem; color: #f1f5f9; }
        .pump-card .status { font-size: 0.75rem; color: #94a3b8; }
        .pump-card.active .status { color: rgba(255,255,255,0.9); }
        
        /* Master Pump Special */
        .master-pump-container {
            background: linear-gradient(180deg, rgba(59, 130, 246, 0.1) 0%, rgba(59, 130, 246, 0.05) 100%);
            border: 1px solid rgba(59, 130, 246, 0.2);
        }
        .master-pump-container .pump-card.active {
            background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
            box-shadow: 0 8px 25px rgba(37, 99, 235, 0.4);
        }
        
        /* Nav Pills */
        .nav-pills .nav-link {
            color: #94a3b8;
            border-radius: 12px;
            padding: 12px 20px;
            font-weight: 600;
            transition: all 0.3s;
        }
        .nav-pills .nav-link.active {
            background: rgba(255,255,255,0.1);
            color: #fff;
            border: 1px solid rgba(255,255,255,0.1);
        }
        
        /* Inputs */
        .form-control {
            background: rgba(15, 23, 42, 0.6);
            border: 1px solid rgba(255,255,255,0.1);
            color: white;
            padding: 12px;
            border-radius: 10px;
        }
        .form-control:focus {
            background: rgba(15, 23, 42, 0.8);
            border-color: #6366f1;
            box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.2);
            color: white;
        }
        .input-group-text {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            color: #94a3b8;
        }
        
        /* Animations */
        .spin { animation: spin 2s linear infinite; }
        @keyframes spin { 100% { transform: rotate(360deg); } }
        
        /* Responsive Grid */
        .pump-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 15px;
        }
    </style>
</head>
<body>
    <!-- Navbar -->
    <nav class="navbar navbar-expand fixed-top">
        <div class="container">
            <a class="navbar-brand d-flex align-items-center" href="#">
                <div class="icon-box bg-success bg-opacity-25 text-success rounded-circle p-2 me-2 d-flex align-items-center justify-content-center" style="width:35px;height:35px">
                    <i class="fas fa-leaf"></i>
                </div>
                <span>SmartFarm</span>
            </a>
            <div class="d-flex align-items-center gap-3">
                <div class="d-none d-md-flex align-items-center text-secondary">
                    <i class="fas fa-wifi me-2" id="wifi-icon"></i>
                    <span id="rssi-text" class="small font-monospace">-- dBm</span>
                </div>
                <div class="text-end">
                    <div id="clock" class="fw-bold font-monospace text-light">--:--:--</div>
                    <div class="d-flex align-items-center justify-content-end gap-2" style="font-size: 0.75rem;">
                        <span id="connStatus" class="badge bg-danger rounded-pill">Offline</span>
                    </div>
                </div>
            </div>
        </div>
    </nav>

    <div class="container" style="margin-top: 80px;">
        
        <!-- Alerts -->
        <div id="rtc-alert" class="alert alert-danger d-none glass-card border-0 text-white mb-4" role="alert">
            <div class="d-flex align-items-center">
                <i class="fas fa-exclamation-triangle fs-4 me-3"></i>
                <div>
                    <strong class="d-block">Cảnh báo hệ thống</strong>
                    <span id="rtc-msg">Không tìm thấy module RTC.</span>
                </div>
            </div>
        </div>

        <!-- Main Tabs -->
        <ul class="nav nav-pills mb-4 justify-content-center gap-2" id="pills-tab" role="tablist">
            <li class="nav-item" role="presentation">
                <button class="nav-link active" id="tab-control" data-bs-toggle="pill" data-bs-target="#pills-home">
                    <i class="fas fa-gamepad me-2"></i>Điều Khiển
                </button>
            </li>
            <li class="nav-item" role="presentation">
                <button class="nav-link" id="tab-config" data-bs-toggle="pill" data-bs-target="#pills-config">
                    <i class="fas fa-cog me-2"></i>Cấu Hình
                </button>
            </li>
            <li class="nav-item" role="presentation">
                <button class="nav-link" id="tab-contact" data-bs-toggle="pill" data-bs-target="#pills-contact">
                    <i class="fas fa-address-card me-2"></i>Liên Hệ
                </button>
            </li>
        </ul>

        <div class="tab-content">
            <!-- TAB CONTROL -->
            <div class="tab-pane fade show active" id="pills-home">
                
                <!-- Mode Switch -->
                <div class="glass-card p-3 mb-4">
                    <div class="d-flex justify-content-between align-items-center">
                        <div>
                            <h6 class="mb-0 text-white"><i class="fas fa-robot me-2 text-primary"></i>Chế độ vận hành</h6>
                            <small class="text-muted" id="mode-text">Đang chạy thủ công</small>
                        </div>
                        <div class="btn-group mode-switch" role="group">
                            <button type="button" class="btn" id="btn-mode-manual" onclick="setMode(0)">Thủ Công</button>
                            <button type="button" class="btn" id="btn-mode-auto" onclick="setMode(1)">Tự Động</button>
                        </div>
                    </div>
                </div>

                <!-- Master Pump -->
                <div class="glass-card master-pump-container p-4 mb-4">
                    <div class="d-flex justify-content-between align-items-center mb-3">
                        <h6 class="mb-0 text-primary fw-bold text-uppercase"><i class="fas fa-server me-2"></i>Bơm Tổng (Master)</h6>
                        <span class="badge bg-primary bg-opacity-25 text-primary" id="master-status-badge">OFF</span>
                    </div>
                    <div class="pump-card" id="btn-master" onclick="toggleMaster()">
                        <div class="d-flex align-items-center">
                            <div class="icon-box">
                                <i class="fas fa-fan pump-icon" id="icon-master"></i>
                            </div>
                            <div class="info">
                                <span class="name">Máy Bơm Chính</span>
                                <span class="status" id="master-text">Nhấn để bật/tắt</span>
                            </div>
                        </div>
                        <div class="form-check form-switch pointer-events-none">
                            <input class="form-check-input" type="checkbox" id="sw-master">
                        </div>
                    </div>
                </div>

                <!-- Slaves Grid -->
                <div class="row g-4">
                    <!-- Khu Vực 1 -->
                    <div class="col-md-6">
                        <div class="glass-card h-100">
                            <div class="card-header d-flex justify-content-between">
                                <span><i class="fas fa-map-marker-alt me-2 text-warning"></i>Khu Vực 1</span>
                                <small class="text-muted">Slave 01</small>
                            </div>
                            <div class="card-body p-3">
                                <div class="pump-grid">
                                    <div class="pump-card" id="btn-s0p0" onclick="togglePump(0,0)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 1</span><span class="status">OFF</span></div>
                                    </div>
                                    <div class="pump-card" id="btn-s0p1" onclick="togglePump(0,1)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 2</span><span class="status">OFF</span></div>
                                    </div>
                                    <div class="pump-card" id="btn-s0p2" onclick="togglePump(0,2)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 3</span><span class="status">OFF</span></div>
                                    </div>
                                    <div class="pump-card" id="btn-s0p3" onclick="togglePump(0,3)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 4</span><span class="status">OFF</span></div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>

                    <!-- Khu Vực 2 -->
                    <div class="col-md-6">
                        <div class="glass-card h-100">
                            <div class="card-header d-flex justify-content-between">
                                <span><i class="fas fa-map-marker-alt me-2 text-info"></i>Khu Vực 2</span>
                                <small class="text-muted">Slave 02</small>
                            </div>
                            <div class="card-body p-3">
                                <div class="pump-grid">
                                    <div class="pump-card" id="btn-s1p0" onclick="togglePump(1,0)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 1</span><span class="status">OFF</span></div>
                                    </div>
                                    <div class="pump-card" id="btn-s1p1" onclick="togglePump(1,1)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 2</span><span class="status">OFF</span></div>
                                    </div>
                                    <div class="pump-card" id="btn-s1p2" onclick="togglePump(1,2)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 3</span><span class="status">OFF</span></div>
                                    </div>
                                    <div class="pump-card" id="btn-s1p3" onclick="togglePump(1,3)">
                                        <div class="icon-box"><i class="fas fa-water"></i></div>
                                        <div class="info"><span class="name">Van 4</span><span class="status">OFF</span></div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- TAB CONFIG -->
            <div class="tab-pane fade" id="pills-config">
                <div class="glass-card p-4">
                    <h5 class="mb-4 text-white"><i class="fas fa-clock me-2 text-warning"></i>Cài đặt hẹn giờ</h5>
                    
                    <div class="row g-4">
                        <div class="col-md-6">
                            <label class="form-label text-muted small text-uppercase fw-bold">Buổi Sáng</label>
                            <div class="input-group">
                                <span class="input-group-text"><i class="fas fa-sun text-warning"></i></span>
                                <input type="number" class="form-control" id="mh" placeholder="HH" min="0" max="23">
                                <span class="input-group-text">:</span>
                                <input type="number" class="form-control" id="mm" placeholder="MM" min="0" max="59">
                            </div>
                        </div>
                        <div class="col-md-6">
                            <label class="form-label text-muted small text-uppercase fw-bold">Buổi Chiều</label>
                            <div class="input-group">
                                <span class="input-group-text"><i class="fas fa-moon text-info"></i></span>
                                <input type="number" class="form-control" id="eh" placeholder="HH" min="0" max="23">
                                <span class="input-group-text">:</span>
                                <input type="number" class="form-control" id="em" placeholder="MM" min="0" max="59">
                            </div>
                        </div>
                        <div class="col-12">
                            <label class="form-label text-muted small text-uppercase fw-bold">Thời gian chạy Bơm Tổng (Phút)</label>
                            <div class="input-group">
                                <span class="input-group-text"><i class="fas fa-hourglass-half"></i></span>
                                <input type="number" class="form-control" id="mt" min="1">
                            </div>
                        </div>
                        <div class="col-12 pt-3">
                            <button class="btn btn-outline-info w-100 py-3 fw-bold rounded-3 mb-3" onclick="syncTime()">
                                <i class="fas fa-sync-alt me-2"></i>ĐỒNG BỘ GIỜ (ĐIỆN THOẠI)
                            </button>
                            <button class="btn btn-primary w-100 py-3 fw-bold rounded-3" onclick="saveConfig()">
                                <i class="fas fa-save me-2"></i>LƯU CÀI ĐẶT
                            </button>
                        </div>
                    </div>
                </div>
            </div>

            <!-- TAB CONTACT -->
            <div class="tab-pane fade" id="pills-contact">
                <div class="glass-card p-4 text-center">
                    <div class="mb-4">
                        <div class="icon-box bg-gradient bg-primary bg-opacity-25 text-white rounded-circle d-inline-flex align-items-center justify-content-center shadow" style="width:90px;height:90px;font-size:2.5rem; background: var(--primary-grad);">
                            <i class="fas fa-user-tie"></i>
                        </div>
                    </div>
                    <h4 class="text-white fw-bold mb-1">Trần Doãn Quảng</h4>
                    <p class="text-muted mb-4">IoT Developer & Smart Farm Solutions</p>
                    
                    <div class="d-grid gap-3">
                        <a href="tel:0971048764" class="btn btn-outline-light p-3 rounded-3 text-start d-flex align-items-center hover-effect">
                            <i class="fas fa-phone-alt me-3 text-success fs-4"></i>
                            <div>
                                <div class="small text-muted text-uppercase fw-bold" style="font-size: 0.7rem;">Hotline / Zalo</div>
                                <div class="fw-bold fs-5">0971 048 764</div>
                            </div>
                        </a>
                        
                        <a href="https://www.facebook.com/trandoanquang.03" target="_blank" class="btn btn-outline-light p-3 rounded-3 text-start d-flex align-items-center hover-effect">
                            <i class="fab fa-facebook me-3 text-primary fs-4"></i>
                            <div>
                                <div class="small text-muted text-uppercase fw-bold" style="font-size: 0.7rem;">Facebook</div>
                                <div class="fw-bold fs-5">Trần Doãn Quảng</div>
                            </div>
                        </a>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js"></script>
    <script>
        let state = { mode: 0, masterPumpStatus: false, pumps: [[0,0,0,0],[0,0,0,0]], rtcFound: true, ntpSynced: false };
        let serverTime = null; // Biến lưu thời gian thực từ ESP32

        function updateUI() {
            // 1. Update Alerts
            const rtcAlert = document.getElementById('rtc-alert');
            const rtcMsg = document.getElementById('rtc-msg');
            if (!state.rtcFound) {
                rtcAlert.classList.remove('d-none');
                if (state.ntpSynced) {
                    rtcAlert.className = 'alert alert-warning glass-card border-0 text-white mb-4';
                    rtcMsg.innerHTML = '<strong>CHẾ ĐỘ NTP:</strong> Đang chạy bằng giờ Internet (Lỗi RTC).';
                } else {
                    rtcAlert.className = 'alert alert-danger glass-card border-0 text-white mb-4';
                    rtcMsg.innerHTML = '<strong>CẢNH BÁO:</strong> Lỗi RTC và chưa có giờ Internet!';
                }
            } else {
                rtcAlert.classList.add('d-none');
            }

            // 2. Update Mode
            const isAuto = state.mode === 1;
            document.getElementById('btn-mode-manual').className = isAuto ? 'btn' : 'btn active';
            document.getElementById('btn-mode-auto').className = isAuto ? 'btn active' : 'btn';
            document.getElementById('mode-text').innerText = isAuto ? 'Hệ thống đang chạy tự động' : 'Điều khiển thủ công';
            
            // Disable buttons in Auto mode
            document.querySelectorAll('.pump-card').forEach(el => {
                if(el.id !== 'btn-master') el.style.pointerEvents = isAuto ? 'none' : 'auto';
                if(isAuto) el.classList.add('opacity-50'); else el.classList.remove('opacity-50');
            });

            // 3. Update Master Pump
            const mBtn = document.getElementById('btn-master');
            const mIcon = document.getElementById('icon-master');
            const mSw = document.getElementById('sw-master');
            
            if(state.masterPumpStatus) {
                mBtn.classList.add('active');
                mIcon.classList.add('spin');
                mSw.checked = true;
                document.getElementById('master-text').innerText = 'Đang hoạt động';
                document.getElementById('master-status-badge').innerText = 'ON';
                document.getElementById('master-status-badge').className = 'badge bg-white text-primary';
            } else {
                mBtn.classList.remove('active');
                mIcon.classList.remove('spin');
                mSw.checked = false;
                document.getElementById('master-text').innerText = 'Đã tắt';
                document.getElementById('master-status-badge').innerText = 'OFF';
                document.getElementById('master-status-badge').className = 'badge bg-primary bg-opacity-25 text-primary';
            }

            // 4. Update Slaves
            for(let s=0; s<2; s++) {
                for(let p=0; p<4; p++) {
                    const btn = document.getElementById(`btn-s${s}p${p}`);
                    const statusTxt = btn.querySelector('.status');
                    if(state.pumps[s][p]) {
                        btn.classList.add('active');
                        statusTxt.innerText = 'ON';
                    } else {
                        btn.classList.remove('active');
                        statusTxt.innerText = 'OFF';
                    }
                }
            }
        }

        async function fetchStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                state = data;
                
                // Cập nhật RSSI
                if (data.rssi !== undefined) {
                    document.getElementById('rssi-text').innerText = data.rssi + ' dBm';
                    const icon = document.getElementById('wifi-icon');
                    icon.className = data.rssi >= -60 ? 'fas fa-wifi text-success me-2' : 
                                     data.rssi >= -80 ? 'fas fa-wifi text-warning me-2' : 'fas fa-wifi text-danger me-2';
                }

                // Cập nhật giờ từ Server
                if (data.ch !== undefined) {
                    const now = new Date();
                    now.setHours(data.ch, data.cm, data.cs);
                    serverTime = now;
                }

                updateUI();
                
                // Update inputs only if not focused
                if(document.activeElement.tagName !== 'INPUT') {
                    document.getElementById('mh').value = data.mh;
                    document.getElementById('mm').value = data.mm;
                    document.getElementById('eh').value = data.eh;
                    document.getElementById('em').value = data.em;
                    document.getElementById('mt').value = data.masterPumpMinutes;
                }
                
                const conn = document.getElementById('connStatus');
                conn.className = 'badge bg-success rounded-pill';
                conn.innerText = 'Online';
                
            } catch(e) {
                const conn = document.getElementById('connStatus');
                conn.className = 'badge bg-danger rounded-pill';
                conn.innerText = 'Offline';
            }
        }

        async function sendControl(payload) {
            try { await fetch('/api/control', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(payload) }); fetchStatus(); } catch(e) {}
        }

        function setMode(m) { sendControl({mode: m}); }
        function toggleMaster() { if(state.mode===0) sendControl({mps: !state.masterPumpStatus}); }
        function togglePump(s, p) { if(state.mode===0) sendControl({s: s, p: p, state: !state.pumps[s][p]}); }
        
        function saveConfig() {
            sendControl({
                mh: parseInt(document.getElementById('mh').value),
                mm: parseInt(document.getElementById('mm').value),
                eh: parseInt(document.getElementById('eh').value),
                em: parseInt(document.getElementById('em').value),
                mt: parseInt(document.getElementById('mt').value)
            });
            alert('Đã lưu cài đặt thành công!');
        }

        function syncTime() {
            const now = new Date();
            const url = `/api/sync_time?y=${now.getFullYear()}&m=${now.getMonth()+1}&d=${now.getDate()}&h=${now.getHours()}&mi=${now.getMinutes()}&s=${now.getSeconds()}`;
            fetch(url).then(res => {
                if(res.ok) alert('Đã đồng bộ thời gian thành công!');
                else alert('Lỗi khi đồng bộ!');
            });
        }

        setInterval(fetchStatus, 2000);
        fetchStatus();
        
        // Đồng hồ chạy theo từng giây dựa trên giờ của ESP32
        setInterval(() => {
            if (serverTime) {
                serverTime.setSeconds(serverTime.getSeconds() + 1);
                document.getElementById('clock').innerText = serverTime.toLocaleTimeString('vi-VN');
            }
        }, 1000);
    </script>
</body>
</html>
)rawliteral";