    def query_results(self, query_type, **kwargs):
        """查询分析结果"""
        if not self.ensure_valid_token():
            raise Exception("Failed to refresh token")

        url = f"{self.base_url}/api/query"
        headers = {
            "Authorization": f"Bearer {self.access_token}",
            "Content-Type": "application/json"
        }

        data = {
            "query_type": query_type
        }
        data.update(kwargs)

        response = requests.post(url, headers=headers, json=data)
        return response.json()

    def get_server_status(self):
        """获取服务器状态"""
        url = f"{self.base_url}/api/status"
        response = requests.get(url)
        return response.json()

# 使用示例
if __name__ == "__main__":
    # 初始化客户端
    client = DoubaoAnalyzerClient(
        base_url="http://your-server:8080",
        username="admin",
        password="admin123"
    )

    # 分析单张图片
    result = client.analyze_media(
        media_type="image",
        media_url="https://example.com/image.jpg"
    )
    print("图片分析结果:", result)

    # 分析单个视频
    result = client.analyze_media(
        media_type="video",
        media_url="https://example.com/video.mp4",
        video_frames=8
    )
    print("视频分析结果:", result)

    # 批量分析
    requests = [
        {
            "media_type": "image",
            "media_url": "https://example.com/image1.jpg"
        },
        {
            "media_type": "video",
            "media_url": "https://example.com/video1.mp4",
            "video_frames": 5
        }
    ]
    result = client.batch_analyze(requests)
    print("批量分析结果:", result)

    # 查询结果
    result = client.query_results(query_type="tag", tag="风景")
    print("查询结果:", result)

    # 获取服务器状态
    status = client.get_server_status()
    print("服务器状态:", status)
```

### JavaScript SDK示例

```javascript
class DoubaoAnalyzerClient {
    constructor(baseUrl, username, password) {
        this.baseUrl = baseUrl;
        this.accessToken = null;
        this.refreshToken = null;
        this.tokenExpiresAt = 0;

        // 登录获取令牌
        this.login(username, password);
    }

    async login(username, password) {
        const url = `${this.baseUrl}/api/auth`;
        const data = {
            username: username,
            password: password
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            });

            const result = await response.json();

            if (result.success) {
                this.accessToken = result.data.access_token;
                this.refreshToken = result.data.refresh_token;
                this.tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
                return true;
            }
            return false;
        } catch (error) {
            console.error('Login error:', error);
            return false;
        }
    }

    async refreshAccessToken() {
        const url = `${this.baseUrl}/api/auth/refresh`;
        const data = {
            refresh_token: this.refreshToken
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            });

            const result = await response.json();

            if (result.success) {
                this.accessToken = result.data.access_token;
                this.refreshToken = result.data.refresh_token;
                this.tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
                return true;
            }
            return false;
        } catch (error) {
            console.error('Token refresh error:', error);
            return false;
        }
    }

    async ensureValidToken() {
        if (Date.now() >= this.tokenExpiresAt) {
            return await this.refreshAccessToken();
        }
        return true;
    }

    async analyzeMedia(mediaType, mediaUrl, options = {}) {
        if (!(await this.ensureValidToken())) {
            throw new Error('Failed to refresh token');
        }

        const url = `${this.baseUrl}/api/analyze`;
        const headers = {
            'Authorization': `Bearer ${this.accessToken}`,
            'Content-Type': 'application/json'
        };

        const data = {
            media_type: mediaType,
            media_url: mediaUrl,
            save_to_db: options.saveToDb !== undefined ? options.saveToDb : true
        };

        if (options.prompt) data.prompt = options.prompt;
        if (options.maxTokens) data.max_tokens = options.maxTokens;
        if (options.videoFrames && mediaType === 'video') data.video_frames = options.videoFrames;

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: headers,
                body: JSON.stringify(data)
            });

            return await response.json();
        } catch (error) {
            console.error('Analysis error:', error);
            throw error;
        }
    }

    async batchAnalyze(requests) {
        if (!(await this.ensureValidToken())) {
            throw new Error('Failed to refresh token');
        }

        const url = `${this.baseUrl}/api/batch_analyze`;
        const headers = {
            'Authorization': `Bearer ${this.accessToken}`,
            'Content-Type': 'application/json'
        };

        const data = {
            requests: requests
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: headers,
                body: JSON.stringify(data)
            });

            return await response.json();
        } catch (error) {
            console.error('Batch analysis error:', error);
            throw error;
        }
    }

    async queryResults(queryType, params = {}) {
        if (!(await this.ensureValidToken())) {
            throw new Error('Failed to refresh token');
        }

        const url = `${this.baseUrl}/api/query`;
        const headers = {
            'Authorization': `Bearer ${this.accessToken}`,
            'Content-Type': 'application/json'
        };

        const data = {
            query_type: queryType,
            ...params
        };

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: headers,
                body: JSON.stringify(data)
            });

            return await response.json();
        } catch (error) {
            console.error('Query error:', error);
            throw error;
        }
    }

    async getServerStatus() {
        const url = `${this.baseUrl}/api/status`;

        try {
            const response = await fetch(url);
            return await response.json();
        } catch (error) {
            console.error('Status query error:', error);
            throw error;
        }
    }
}

// 使用示例
(async () => {
    // 初始化客户端
    const client = new DoubaoAnalyzerClient(
        'http://your-server:8080',
        'admin',
        'admin123'
    );

    try {
        // 分析单张图片
        const imageResult = await client.analyzeMedia(
            'image',
            'https://example.com/image.jpg'
        );
        console.log('图片分析结果:', imageResult);

        // 分析单个视频
        const videoResult = await client.analyzeMedia(
            'video',
            'https://example.com/video.mp4',
            { videoFrames: 8 }
        );
        console.log('视频分析结果:', videoResult);

        // 批量分析
        const requests = [
            {
                media_type: 'image',
                media_url: 'https://example.com/image1.jpg'
            },
            {
                media_type: 'video',
                media_url: 'https://example.com/video1.mp4',
                video_frames: 5
            }
        ];
        const batchResult = await client.batchAnalyze(requests);
        console.log('批量分析结果:', batchResult);

        // 查询结果
        const queryResult = await client.queryResults('tag', { tag: '风景' });
        console.log('查询结果:', queryResult);

        // 获取服务器状态
        const status = await client.getServerStatus();
        console.log('服务器状态:', status);
    } catch (error) {
        console.error('操作失败:', error);
    }
})();
```

## 集成指南

### Web应用集成

#### 1. 基本集成步骤

1. **获取API访问凭证**:
   - 联系系统管理员获取API访问账号
   - 获取API服务器地址和端口

2. **集成认证机制**:
   - 实现JWT令牌获取和刷新逻辑
   - 在请求中添加Authorization头

3. **实现媒体分析功能**:
   - 调用`/api/analyze`接口分析单个媒体
   - 调用`/api/batch_analyze`接口批量分析媒体

4. **实现结果查询功能**:
   - 调用`/api/query`接口查询分析结果
   - 根据业务需求选择合适的查询方式

#### 2. 前端集成示例

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>媒体分析示例</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        .container {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }
        .section {
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 20px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }
        input, select, textarea {
            width: 100%;
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 15px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background-color: #45a049;
        }
        .result {
            margin-top: 20px;
            padding: 15px;
            background-color: #f9f9f9;
            border-radius: 4px;
            white-space: pre-wrap;
        }
        .error {
            color: #d9534f;
        }
        .success {
            color: #5cb85c;
        }
        .hidden {
            display: none;
        }
        .tabs {
            display: flex;
            border-bottom: 1px solid #ddd;
        }
        .tab {
            padding: 10px 20px;
            cursor: pointer;
            border: none;
            background: none;
        }
        .tab.active {
            border-bottom: 2px solid #4CAF50;
            font-weight: bold;
        }
        .tab-content {
            padding: 20px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>豆包媒体分析系统</h1>

        <!-- 登录部分 -->
        <div class="section" id="loginSection">
            <h2>系统登录</h2>
            <div class="form-group">
                <label for="username">用户名:</label>
                <input type="text" id="username" value="admin">
            </div>
            <div class="form-group">
                <label for="password">密码:</label>
                <input type="password" id="password" value="admin123">
            </div>
            <button id="loginBtn">登录</button>
            <div id="loginResult" class="result"></div>
        </div>

        <!-- 功能选项卡 -->
        <div class="section hidden" id="mainSection">
            <div class="tabs">
                <button class="tab active" data-tab="single">单媒体分析</button>
                <button class="tab" data-tab="batch">批量分析</button>
                <button class="tab" data-tab="query">结果查询</button>
                <button class="tab" data-tab="status">系统状态</button>
            </div>

            <!-- 单媒体分析 -->
            <div class="tab-content" id="singleTab">
                <h2>单媒体分析</h2>
                <div class="form-group">
                    <label for="mediaType">媒体类型:</label>
                    <select id="mediaType">
                        <option value="image">图片</option>
                        <option value="video">视频</option>
                    </select>
                </div>
                <div class="form-group">
                    <label for="mediaUrl">媒体URL:</label>
                    <input type="text" id="mediaUrl" placeholder="https://example.com/image.jpg">
                </div>
                <div class="form-group" id="videoFramesGroup" style="display: none;">
                    <label for="videoFrames">视频帧数:</label>
                    <input type="number" id="videoFrames" value="5" min="1" max="20">
                </div>
                <div class="form-group">
                    <label for="prompt">自定义提示词 (可选):</label>
                    <textarea id="prompt" rows="3" placeholder="留空使用默认提示词"></textarea>
                </div>
                <div class="form-group">
                    <label for="maxTokens">最大令牌数:</label>
                    <input type="number" id="maxTokens" value="1500" min="100" max="4000">
                </div>
                <div class="form-group">
                    <label>
                        <input type="checkbox" id="saveToDb" checked> 保存结果到数据库
                    </label>
                </div>
                <button id="analyzeBtn">分析</button>
                <div id="analyzeResult" class="result"></div>
            </div>

            <!-- 批量分析 -->
            <div class="tab-content hidden" id="batchTab">
                <h2>批量分析</h2>
                <div id="batchItems">
                    <div class="batch-item">
                        <div class="form-group">
                            <label>媒体类型:</label>
                            <select class="batchMediaType">
                                <option value="image">图片</option>
                                <option value="video">视频</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>媒体URL:</label>
                            <input type="text" class="batchMediaUrl" placeholder="https://example.com/image.jpg">
                        </div>
                        <div class="form-group batchVideoFramesGroup" style="display: none;">
                            <label>视频帧数:</label>
                            <input type="number" class="batchVideoFrames" value="5" min="1" max="20">
                        </div>
                    </div>
                </div>
                <button id="addBatchItemBtn">添加项目</button>
                <button id="batchAnalyzeBtn">批量分析</button>
                <div id="batchAnalyzeResult" class="result"></div>
            </div>

            <!-- 结果查询 -->
            <div class="tab-content hidden" id="queryTab">
                <h2>结果查询</h2>
                <div class="form-group">
                    <label for="queryType">查询类型:</label>
                    <select id="queryType">
                        <option value="all">全部</option>
                        <option value="tag">按标签</option>
                        <option value="type">按类型</option>
                        <option value="date_range">按日期范围</option>
                        <option value="recent">最近记录</option>
                        <option value="url">按URL</option>
                    </select>
                </div>

                <!-- 按标签查询 -->
                <div class="form-group hidden" id="tagQueryGroup">
                    <label for="tagQuery">标签:</label>
                    <input type="text" id="tagQuery" placeholder="风景">
                </div>

                <!-- 按类型查询 -->
                <div class="form-group hidden" id="typeQueryGroup">
                    <label for="typeQuery">文件类型:</label>
                    <select id="typeQuery">
                        <option value="image">图片</option>
                        <option value="video">视频</option>
                    </select>
                </div>

                <!-- 按日期范围查询 -->
                <div class="form-group hidden" id="dateRangeQueryGroup">
                    <label for="startDateQuery">开始日期:</label>
                    <input type="date" id="startDateQuery">
                    <label for="endDateQuery">结束日期:</label>
                    <input type="date" id="endDateQuery">
                </div>

                <!-- 最近记录 -->
                <div class="form-group hidden" id="recentQueryGroup">
                    <label for="recentLimitQuery">记录数量:</label>
                    <input type="number" id="recentLimitQuery" value="10" min="1" max="100">
                </div>

                <!-- 按URL查询 -->
                <div class="form-group hidden" id="urlQueryGroup">
                    <label for="urlQuery">媒体URL:</label>
                    <input type="text" id="urlQuery" placeholder="https://example.com/image.jpg">
                </div>

                <!-- 全部查询 -->
                <div class="form-group hidden" id="allQueryGroup">
                    <label for="allConditionQuery">自定义条件:</label>
                    <textarea id="allConditionQuery" rows="3" placeholder="例如: file_type='image' AND created_at > '2023-07-01'"></textarea>
                </div>

                <button id="queryBtn">查询</button>
                <div id="queryResult" class="result"></div>
            </div>

            <!-- 系统状态 -->
            <div class="tab-content hidden" id="statusTab">
                <h2>系统状态</h2>
                <button id="statusBtn">获取状态</button>
                <div id="statusResult" class="result"></div>
            </div>
        </div>
    </div>

    <script>
        // 全局变量
        let accessToken = null;
        let refreshToken = null;
        let tokenExpiresAt = 0;
        const API_BASE_URL = 'http://localhost:8080/api';

        // DOM元素
        const loginSection = document.getElementById('loginSection');
        const mainSection = document.getElementById('mainSection');
        const loginBtn = document.getElementById('loginBtn');
        const loginResult = document.getElementById('loginResult');

        // 单媒体分析
        const mediaType = document.getElementById('mediaType');
        const mediaUrl = document.getElementById('mediaUrl');
        const videoFramesGroup = document.getElementById('videoFramesGroup');
        const videoFrames = document.getElementById('videoFrames');
        const prompt = document.getElementById('prompt');
        const maxTokens = document.getElementById('maxTokens');
        const saveToDb = document.getElementById('saveToDb');
        const analyzeBtn = document.getElementById('analyzeBtn');
        const analyzeResult = document.getElementById('analyzeResult');

        // 批量分析
        const batchItems = document.getElementById('batchItems');
        const addBatchItemBtn = document.getElementById('addBatchItemBtn');
        const batchAnalyzeBtn = document.getElementById('batchAnalyzeBtn');
        const batchAnalyzeResult = document.getElementById('batchAnalyzeResult');

        // 查询
        const queryType = document.getElementById('queryType');
        const tagQueryGroup = document.getElementById('tagQueryGroup');
        const tagQuery = document.getElementById('tagQuery');
        const typeQueryGroup = document.getElementById('typeQueryGroup');
        const typeQuery = document.getElementById('typeQuery');
        const dateRangeQueryGroup = document.getElementById('dateRangeQueryGroup');
        const startDateQuery = document.getElementById('startDateQuery');
        const endDateQuery = document.getElementById('endDateQuery');
        const recentQueryGroup = document.getElementById('recentQueryGroup');
        const recentLimitQuery = document.getElementById('recentLimitQuery');
        const urlQueryGroup = document.getElementById('urlQueryGroup');
        const urlQuery = document.getElementById('urlQuery');
        const allQueryGroup = document.getElementById('allQueryGroup');
        const allConditionQuery = document.getElementById('allConditionQuery');
        const queryBtn = document.getElementById('queryBtn');
        const queryResult = document.getElementById('queryResult');

        // 系统状态
        const statusBtn = document.getElementById('statusBtn');
        const statusResult = document.getElementById('statusResult');

        // 选项卡切换
        const tabs = document.querySelectorAll('.tab');
        const tabContents = document.querySelectorAll('.tab-content');

        // 初始化
        function init() {
            // 绑定事件
            loginBtn.addEventListener('click', handleLogin);
            mediaType.addEventListener('change', handleMediaTypeChange);
            analyzeBtn.addEventListener('click', handleAnalyze);
            addBatchItemBtn.addEventListener('click', addBatchItem);
            batchAnalyzeBtn.addEventListener('click', handleBatchAnalyze);
            queryType.addEventListener('change', handleQueryTypeChange);
            queryBtn.addEventListener('click', handleQuery);
            statusBtn.addEventListener('click', handleStatus);

            // 选项卡切换事件
            tabs.forEach(tab => {
                tab.addEventListener('click', () => {
                    const tabName = tab.getAttribute('data-tab');
                    switchTab(tabName);
                });
            });

            // 批量分析项目媒体类型变化事件
            document.addEventListener('change', (e) => {
                if (e.target.classList.contains('batchMediaType')) {
                    const batchItem = e.target.closest('.batch-item');
                    const videoFramesGroup = batchItem.querySelector('.batchVideoFramesGroup');

                    if (e.target.value === 'video') {
                        videoFramesGroup.style.display = 'block';
                    } else {
                        videoFramesGroup.style.display = 'none';
                    }
                }
            });
        }

        // 登录处理
        async function handleLogin() {
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;

            try {
                const response = await fetch(`${API_BASE_URL}/auth`, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        username: username,
                        password: password
                    })
                });

                const result = await response.json();

                if (result.success) {
                    accessToken = result.data.access_token;
                    refreshToken = result.data.refresh_token;
                    tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新

                    loginResult.innerHTML = `<div class="success">登录成功!</div>`;
                    loginSection.classList.add('hidden');
                    mainSection.classList.remove('hidden');

                    // 设置自动刷新令牌
                    setupTokenRefresh();
                } else {
                    loginResult.innerHTML = `<div class="error">登录失败: ${result.message}</div>`;
                }
            } catch (error) {
                loginResult.innerHTML = `<div class="error">登录错误: ${error.message}</div>`;
            }
        }

        // 设置令牌自动刷新
        function setupTokenRefresh() {
            // 每5分钟检查一次令牌是否需要刷新
            setInterval(async () => {
                if (Date.now() >= tokenExpiresAt) {
                    try {
                        const response = await fetch(`${API_BASE_URL}/auth/refresh`, {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json'
                            },
                            body: JSON.stringify({
                                refresh_token: refreshToken
                            })
                        });

                        const result = await response.json();

                        if (result.success) {
                            accessToken = result.data.access_token;
                            refreshToken = result.data.refresh_token;
                            tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
                            console.log('令牌已刷新');
                        } else {
                            console.error('令牌刷新失败:', result.message);
                            // 可以在这里实现重新登录逻辑
                        }
                    } catch (error) {
                        console.error('令牌刷新错误:', error);
                    }
                }
            }, 5 * 60 * 1000); // 5分钟
        }

        // 媒体类型变化处理
        function handleMediaTypeChange() {
            if (mediaType.value === 'video') {
                videoFramesGroup.style.display = 'block';
            } else {
                videoFramesGroup.style.display = 'none';
            }
        }

        // 单媒体分析处理
        async function handleAnalyze() {
            const mediaTypeValue = mediaType.value;
            const mediaUrlValue = mediaUrl.value.trim();

            if (!mediaUrlValue) {
                analyzeResult.innerHTML = `<div class="error">请输入媒体URL</div>`;
                return;
            }

            const data = {
                media_type: mediaTypeValue,
                media_url: mediaUrlValue,
                save_to_db: saveToDb.checked
            };

            if (prompt.value.trim()) {
                data.prompt = prompt.value.trim();
            }

            if (maxTokens.value) {
                data.max_tokens = parseInt(maxTokens.value);
            }

            if (mediaTypeValue === 'video' && videoFrames.value) {
                data.video_frames = parseInt(videoFrames.value);
            }

            try {
                analyzeBtn.disabled = true;
                analyzeBtn.textContent = '分析中...';

                const response = await fetch(`${API_BASE_URL}/analyze`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>响应时间: ${result.response_time}秒</div>`;

                    if (result.data.content) {
                        html += `<div>分析结果: ${result.data.content}</div>`;
                    }

                    if (result.data.tags && result.data.tags.length > 0) {
                        html += `<div>标签: ${result.data.tags.join(', ')}</div>`;
                    }

                    if (result.data.usage) {
                        html += `<div>令牌使用: ${JSON.stringify(result.data.usage)}</div>`;
                    }

                    analyzeResult.innerHTML = html;
                } else {
                    analyzeResult.innerHTML = `<div class="error">分析失败: ${result.message}</div>`;
                }
            } catch (error) {
                analyzeResult.innerHTML = `<div class="error">分析错误: ${error.message}</div>`;
            } finally {
                analyzeBtn.disabled = false;
                analyzeBtn.textContent = '分析';
            }
        }

        // 添加批量分析项目
        function addBatchItem() {
            const batchItem = document.createElement('div');
            batchItem.className = 'batch-item';
            batchItem.innerHTML = `
                <div class="form-group">
                    <label>媒体类型:</label>
                    <select class="batchMediaType">
                        <option value="image">图片</option>
                        <option value="video">视频</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>媒体URL:</label>
                    <input type="text" class="batchMediaUrl" placeholder="https://example.com/image.jpg">
                </div>
                <div class="form-group batchVideoFramesGroup" style="display: none;">
                    <label>视频帧数:</label>
                    <input type="number" class="batchVideoFrames" value="5" min="1" max="20">
                </div>
                <button type="button" class="removeBatchItem">删除</button>
            `;

            batchItems.appendChild(batchItem);

            // 添加删除按钮事件
            batchItem.querySelector('.removeBatchItem').addEventListener('click', () => {
                batchItem.remove();
            });
        }

        // 批量分析处理
        async function handleBatchAnalyze() {
            const batchItemsElements = document.querySelectorAll('.batch-item');

            if (batchItemsElements.length === 0) {
                batchAnalyzeResult.innerHTML = `<div class="error">请添加至少一个分析项目</div>`;
                return;
            }

            const requests = [];

            for (const item of batchItemsElements) {
                const mediaType = item.querySelector('.batchMediaType').value;
                const mediaUrl = item.querySelector('.batchMediaUrl').value.trim();

                if (!mediaUrl) {
                    batchAnalyzeResult.innerHTML = `<div class="error">所有项目都必须提供媒体URL</div>`;
                    return;
                }

                const request = {
                    media_type: mediaType,
                    media_url: mediaUrl,
                    save_to_db: true
                };

                if (mediaType === 'video') {
                    const videoFrames = item.querySelector('.batchVideoFrames').value;
                    if (videoFrames) {
                        request.video_frames = parseInt(videoFrames);
                    }
                }

                requests.push(request);
            }

            try {
                batchAnalyzeBtn.disabled = true;
                batchAnalyzeBtn.textContent = '分析中...';

                const response = await fetch(`${API_BASE_URL}/batch_analyze`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        requests: requests
                    })
                });

                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>总响应时间: ${result.response_time}秒</div>`;

                    if (result.data.summary) {
                        html += `<div>统计: ${JSON.stringify(result.data.summary)}</div>`;
                    }

                    if (result.data.results && result.data.results.length > 0) {
                        html += '<div>详细结果:</div>';

                        for (const item of result.data.results) {
                            html += `<div style="margin-left: 20px; margin-top: 10px;">`;
                            html += `<div>任务ID: ${item.task_id}</div>`;

                            if (item.success) {
                                html += `<div class="success">成功</div>`;

                                if (item.content) {
                                    html += `<div>分析结果: ${item.content}</div>`;
                                }

                                if (item.tags && item.tags.length > 0) {
                                    html += `<div>标签: ${item.tags.join(', ')}</div>`;
                                }

                                if (item.usage) {
                                    html += `<div>令牌使用: ${JSON.stringify(item.usage)}</div>`;
                                }
                            } else {
                                html += `<div class="error">失败: ${item.error}</div>`;
                            }

                            html += `</div>`;
                        }
                    }

                    batchAnalyzeResult.innerHTML = html;
                } else {
                    batchAnalyzeResult.innerHTML = `<div class="error">批量分析失败: ${result.message}</div>`;
                }
            } catch (error) {
                batchAnalyzeResult.innerHTML = `<div class="error">批量分析错误: ${error.message}</div>`;
            } finally {
                batchAnalyzeBtn.disabled = false;
                batchAnalyzeBtn.textContent = '批量分析';
            }
        }

        // 查询类型变化处理
        function handleQueryTypeChange() {
            // 隐藏所有查询组
            tagQueryGroup.classList.add('hidden');
            typeQueryGroup.classList.add('hidden');
            dateRangeQueryGroup.classList.add('hidden');
            recentQueryGroup.classList.add('hidden');
            urlQueryGroup.classList.add('hidden');
            allQueryGroup.classList.add('hidden');

            // 根据选择的查询类型显示对应的组
            switch (queryType.value) {
                case 'tag':
                    tagQueryGroup.classList.remove('hidden');
                    break;
                case 'type':
                    typeQueryGroup.classList.remove('hidden');
                    break;
                case 'date_range':
                    dateRangeQueryGroup.classList.remove('hidden');
                    break;
                case 'recent':
                    recentQueryGroup.classList.remove('hidden');
                    break;
                case 'url':
                    urlQueryGroup.classList.remove('hidden');
                    break;
                case 'all':
                    allQueryGroup.classList.remove('hidden');
                    break;
            }
        }

        // 查询处理
        async function handleQuery() {
            const queryTypeValue = queryType.value;
            const data = {
                query_type: queryTypeValue
            };

            // 根据查询类型添加相应的参数
            switch (queryTypeValue) {
                case 'tag':
                    const tagValue = tagQuery.value.trim();
                    if (!tagValue) {
                        queryResult.innerHTML = `<div class="error">请输入要查询的标签</div>`;
                        return;
                    }
                    data.tag = tagValue;
                    break;

                case 'type':
                    data.file_type = typeQuery.value;
                    break;

                case 'date_range':
                    const startDateValue = startDateQuery.value;
                    const endDateValue = endDateQuery.value;

                    if (!startDateValue || !endDateValue) {
                        queryResult.innerHTML = `<div class="error">请选择开始和结束日期</div>`;
                        return;
                    }

                    data.start_date = startDateValue;
                    data.end_date = endDateValue;
                    break;

                case 'recent':
                    data.limit = parseInt(recentLimitQuery.value) || 10;
                    break;

                case 'url':
                    const urlValue = urlQuery.value.trim();
                    if (!urlValue) {
                        queryResult.innerHTML = `<div class="error">请输入要查询的URL</div>`;
                        return;
                    }
                    data.media_url = urlValue;
                    break;

                case 'all':
                    const conditionValue = allConditionQuery.value.trim();
                    if (conditionValue) {
                        data.condition = conditionValue;
                    }
                    break;
            }

            try {
                queryBtn.disabled = true;
                queryBtn.textContent = '查询中...';

                const response = await fetch(`${API_BASE_URL}/query`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>响应时间: ${result.response_time}秒</div>`;

                    if (result.data.results && result.data.results.length > 0) {
                        html += '<div>查询结果:</div>';

                        for (const item of result.data.results) {
                            html += `<div style="margin-left: 20px; margin-top: 10px; border: 1px solid #ddd; padding: 10px;">`;
                            html += `<div>ID: ${item.id}</div>`;
                            html += `<div>文件路径: ${item.file_path}</div>`;
                            html += `<div>文件名: ${item.file_name}</div>`;
                            html += `<div>文件类型: ${item.file_type}</div>`;
                            html += `<div>分析结果: ${item.analysis_result}</div>`;
                            html += `<div>标签: ${item.tags}</div>`;
                            html += `<div>响应时间: ${item.response_time}秒</div>`;
                            html += `<div>创建时间: ${item.created_at}</div>`;
                            html += `</div>`;
                        }
                    } else {
                        html += '<div>没有找到匹配的记录</div>';
                    }

                    queryResult.innerHTML = html;
                } else {
                    queryResult.innerHTML = `<div class="error">查询失败: ${result.message}</div>`;
                }
            } catch (error) {
                queryResult.innerHTML = `<div class="error">查询错误: ${error.message}</div>`;
            } finally {
                queryBtn.disabled = false;
                queryBtn.textContent = '查询';
            }
        }

        // 系统状态处理
        async function handleStatus() {
            try {
                statusBtn.disabled = true;
                statusBtn.textContent = '获取中...';

                const response = await fetch(`${API_BASE_URL}/status`);
                const result = await response.json();

                if (result.success) {
                    let html = `<div class="success">${result.message}</div>`;
                    html += `<div>服务器状态: ${result.data.server_status}</div>`;
                    html += `<div>API密钥已设置: ${result.data.api_key_set ? '是' : '否'}</div>`;
                    html += `<div>监听地址: ${result.data.host}:${result.data.port}</div>`;

                    if (result.data.database_stats) {
                        html += '<div>数据库统计:</div>';
                        html += `<div style="margin-left: 20px;">总记录数: ${result.data.database_stats.total_records}</div>`;
                        html += `<div style="margin-left: 20px;">图片记录数: ${result.data.database_stats.image_records}</div>`;
                        html += `<div style="margin-left: 20px;">视频记录数: ${result.data.database_stats.video_records}</div>`;
                        html += `<div style="margin-left: 20px;">最后更新: ${result.data.database_stats.last_updated}</div>`;
                    }

                    statusResult.innerHTML = html;
                } else {
                    statusResult.innerHTML = `<div class="error">获取状态失败: ${result.message}</div>`;
                }
            } catch (error) {
                statusResult.innerHTML = `<div class="error">获取状态错误: ${error.message}</div>`;
            } finally {
                statusBtn.disabled = false;
                statusBtn.textContent = '获取状态';
            }
        }

        // 选项卡切换
        function switchTab(tabName) {
            // 更新选项卡样式
            tabs.forEach(tab => {
                if (tab.getAttribute('data-tab') === tabName) {
                    tab.classList.add('active');
                } else {
                    tab.classList.remove('active');
                }
            });

            // 显示对应的内容
            tabContents.forEach(content => {
                if (content.id === `${tabName}Tab`) {
                    content.classList.remove('hidden');
                } else {
                    content.classList.add('hidden');
                }
            });
        }

        // 页面加载完成后初始化
        document.addEventListener('DOMContentLoaded', init);
    </script>
</body>
</html>
```

### 移动应用集成

#### 1. React Native集成示例

```javascript
import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TextInput,
  Button,
  StyleSheet,
  ScrollView,
  Alert,
  ActivityIndicator,
  Picker
} from 'react-native';

class DoubaoAnalyzerClient {
  constructor(baseUrl) {
    this.baseUrl = baseUrl;
    this.accessToken = null;
    this.refreshToken = null;
    this.tokenExpiresAt = 0;
  }

  async login(username, password) {
    const url = `${this.baseUrl}/api/auth`;
    const data = {
      username: username,
      password: password
    };

    try {
      const response = await fetch(url, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
      });

      const result = await response.json();

      if (result.success) {
        this.accessToken = result.data.access_token;
        this.refreshToken = result.data.refresh_token;
        this.tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
        return true;
      }
      return false;
    } catch (error) {
      console.error('Login error:', error);
      return false;
    }
  }

  async refreshAccessToken() {
    const url = `${this.baseUrl}/api/auth/refresh`;
    const data = {
      refresh_token: this.refreshToken
    };

    try {
      const response = await fetch(url, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
      });

      const result = await response.json();

      if (result.success) {
        this.accessToken = result.data.access_token;
        this.refreshToken = result.data.refresh_token;
        this.tokenExpiresAt = Date.now() + (result.data.expires_in - 60) * 1000; // 提前1分钟刷新
        return true;
      }
      return false;
    } catch (error) {
      console.error('Token refresh error:', error);
      return false;
    }
  }

  async ensureValidToken() {
    if (Date.now() >= this.tokenExpiresAt) {
      return await this.refreshAccessToken();
    }
    return true;
  }

  async analyzeMedia(mediaType, mediaUrl, options = {}) {
    if (!(await this.ensureValidToken())) {
      throw new Error('Failed to refresh token');
    }

    const url = `${this.baseUrl}/api/analyze`;
    const headers = {
      'Authorization': `Bearer ${this.accessToken}`,
      'Content-Type': 'application/json'
    };

    const data = {
      media_type: mediaType,
      media_url: mediaUrl,
      save_to_db: options.saveToDb !== undefined ? options.saveToDb : true
    };

    if (options.prompt) data.prompt = options.prompt;
    if (options.maxTokens) data.max_tokens = options.maxTokens;
    if (options.videoFrames && mediaType === 'video') data.video_frames = options.videoFrames;

    try {
      const response = await fetch(url, {
        method: 'POST',
        headers: headers,
        body: JSON.stringify(data)
      });

      return await response.json();
    } catch (error) {
      console.error('Analysis error:', error);
      throw error;
    }
  }
}

const MediaAnalyzerApp = () => {
  const [client] = useState(() => new DoubaoAnalyzerClient('http://your-server:8080/api'));
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [mediaType, setMediaType] = useState('image');
  const [mediaUrl, setMediaUrl] = useState('');
  const [prompt, setPrompt] = useState('');
  const [maxTokens, setMaxTokens] = useState('1500');
  const [videoFrames, setVideoFrames] = useState('5');
  const [result, setResult] = useState(null);
  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    // 设置令牌自动刷新
    const interval = setInterval(async () => {
      if (isLoggedIn && Date.now() >= client.tokenExpiresAt) {
        const refreshed = await client.refreshAccessToken();
        if (!refreshed) {
          Alert.alert('错误', '令牌刷新失败，请重新登录');
          setIsLoggedIn(false);
        }
      }
    }, 5 * 60 * 1000); // 5分钟

    return () => clearInterval(interval);
  }, [isLoggedIn, client]);

  const handleLogin = async () => {
    if (!username || !password) {
      Alert.alert('错误', '请输入用户名和密码');
      return;
    }

    setIsLoading(true);
    try {
      const success = await client.login(username, password);
      if (success) {
        setIsLoggedIn(true);
      } else {
        Alert.alert('错误', '登录失败，请检查用户名和密码');
      }
    } catch (error) {
      Alert.alert('错误', `登录错误: ${error.message}`);
    } finally {
      setIsLoading(false);
    }
  };

  const handleAnalyze = async () => {
    if (!mediaUrl) {
      Alert.alert('错误', '请输入媒体URL');
      return;
    }

    setIsLoading(true);
    try {
      const options = {
        prompt: prompt || undefined,
        maxTokens: parseInt(maxTokens) || undefined,
        videoFrames: mediaType === 'video' ? parseInt(videoFrames) || undefined : undefined
      };

      const response = await client.analyzeMedia(mediaType, mediaUrl, options);
      setResult(response);
    } catch (error) {
      Alert.alert('错误', `分析错误: ${error.message}`);
    } finally {
      setIsLoading(false);
    }
  };

  if (!isLoggedIn) {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>豆包媒体分析系统</Text>
        <TextInput
          style={styles.input}
          placeholder="用户名"
          value={username}
          onChangeText={setUsername}
          autoCapitalize="none"
        />
        <TextInput
          style={styles.input}
          placeholder="密码"
          value={password}
          onChangeText={setPassword}
          secureTextEntry
        />
        <Button
          title={isLoading ? "登录中..." : "登录"}
          onPress={handleLogin}
          disabled={isLoading}
        />
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>豆包媒体分析系统</Text>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>媒体分析</Text>

        <Text style={styles.label}>媒体类型:</Text>
        <Picker
          selectedValue={mediaType}
          onValueChange={setMediaType}
          style={styles.picker}
        >
          <Picker.Item label="图片" value="image" />
          <Picker.Item label="视频" value="video" />
        </Picker>

        <Text style={styles.label}>媒体URL:</Text>
        <TextInput
          style={styles.input}
          placeholder="https://example.com/image.jpg"
          value={mediaUrl}
          onChangeText={setMediaUrl}
          autoCapitalize="none"
        />

        {mediaType === 'video' && (
          <>
            <Text style={styles.label}>视频帧数:</Text>
            <TextInput
              style={styles.input}
              value={videoFrames}
              onChangeText={setVideoFrames}
              keyboardType="numeric"
            />
          </>
        )}

        <Text style={styles.label}>自定义提示词 (可选):</Text>
        <TextInput
          style={[styles.input, styles.textArea]}
          placeholder="留空使用默认提示词"
          value={prompt}
          onChangeText={setPrompt}
          multiline
        />

        <Text style={styles.label}>最大令牌数:</Text>
        <TextInput
          style={styles.input}
          value={maxTokens}
          onChangeText={setMaxTokens}
          keyboardType="numeric"
        />

        <Button
          title={isLoading ? "分析中..." : "分析"}
          onPress={handleAnalyze}
          disabled={isLoading}
        />
      </View>

      {isLoading && (
        <View style={styles.loadingContainer}>
          <ActivityIndicator size="large" color="#0000ff" />
          <Text>处理中，请稍候...</Text>
        </View>
      )}

      {result && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>分析结果</Text>
          <Text>成功: {result.success ? '是' : '否'}</Text>
          <Text>消息: {result.message}</Text>
          <Text>响应时间: {result.response_time}秒</Text>

          {result.data && (
            <>
              {result.data.content && (
                <Text>分析结果: {result.data.content}</Text>
              )}

              {result.data.tags && result.data.tags.length > 0 && (
                <Text>标签: {result.data.tags.join(', ')}</Text>
              )}

              {result.data.usage && (
                <Text>令牌使用: {JSON.stringify(result.data.usage)}</Text>
              )}
            </>
          )}
        </View>
      )}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 20,
    backgroundColor: '#f5f5f5'
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    textAlign: 'center',
    marginBottom: 20
  },
  section: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 2,
    elevation: 2
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 10
  },
  label: {
    fontSize: 16,
    marginBottom: 5
  },
  input: {
    borderWidth: 1,
    borderColor: '#ddd',
    padding: 10,
    borderRadius: 4,
    marginBottom: 15,
    fontSize: 16
  },
  textArea: {
    height: 80
  },
  picker: {
    height: 50,
    marginBottom: 15
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20
  }
});

export default MediaAnalyzerApp;
```

### 后端系统集成

#### 1. Java Spring Boot集成示例

```java
// DoubaoAnalyzerService.java
@Service
public class DoubaoAnalyzerService {
    private final RestTemplate restTemplate;
    private final String baseUrl;
    private String accessToken;
    private String refreshToken;
    private long tokenExpiresAt;

    @Autowired
    public DoubaoAnalyzerService(@Value("${doubao.api.url}") String baseUrl) {
        this.baseUrl = baseUrl;
        this.restTemplate = new RestTemplate();
    }

    public boolean login(String username, String password) {
        String url = baseUrl + "/api/auth";

        Map<String, String> request = new HashMap<>();
        request.put("username", username);
        request.put("password", password);

        try {
            ResponseEntity<Map> response = restTemplate.postForEntity(url, request, Map.class);
            Map<String, Object> body = response.getBody();

            if (body != null && Boolean.TRUE.equals(body.get("success"))) {
                Map<String, Object> data = (Map<String, Object>) body.get("data");
                this.accessToken = (String) data.get("access_token");
                this.refreshToken = (String) data.get("refresh_token");
                this.tokenExpiresAt = System.currentTimeMillis() + 
                    ((Integer) data.get("expires_in") - 60) * 1000; // 提前1分钟刷新
                return true;
            }
            return false;
        } catch (Exception e) {
            throw new RuntimeException("登录失败", e);
        }
    }

    public boolean refreshAccessToken() {
        String url = baseUrl + "/api/auth/refresh";

        Map<String, String> request = new HashMap<>();
        request.put("refresh_token", refreshToken);

        try {
            ResponseEntity<Map> response = restTemplate.postForEntity(url, request, Map.class);
            Map<String, Object> body = response.getBody();

            if (body != null && Boolean.TRUE.equals(body.get("success"))) {
                Map<String, Object> data = (Map<String, Object>) body.get("data");
                this.accessToken = (String) data.get("access_token");
                this.refreshToken = (String) data.get("refresh_token");
                this.tokenExpiresAt = System.currentTimeMillis() + 
                    ((Integer) data.get("expires_in") - 60) * 1000; // 提前1分钟刷新
                return true;
            }
            return false;
        } catch (Exception e) {
            throw new RuntimeException("令牌刷新失败", e);
        }
    }

    public boolean ensureValidToken() {
        if (System.currentTimeMillis() >= tokenExpiresAt) {
            return refreshAccessToken();
        }
        return true;
    }

    public Map<String, Object> analyzeMedia(String mediaType, String mediaUrl, 
                                           String prompt, Integer maxTokens, 
                                           Integer videoFrames, Boolean saveToDb) {
        if (!ensureValidToken()) {
            throw new RuntimeException("令牌无效且刷新失败");
        }

        String url = baseUrl + "/api/analyze";

        Map<String, Object> request = new HashMap<>();
        request.put("media_type", mediaType);
        request.put("media_url", mediaUrl);
        request.put("save_to_db", saveToDb != null ? saveToDb : true);

        if (prompt != null && !prompt.isEmpty()) {
            request.put("prompt", prompt);
        }

        if (maxTokens != null) {
            request.put("max_tokens", maxTokens);
        }

        if ("video".equals(mediaType) && videoFrames != null) {
            request.put("video_frames", videoFrames);
        }

        HttpHeaders headers = new HttpHeaders();
        headers.setBearerAuth(accessToken);
        headers.setContentType(MediaType.APPLICATION_JSON);

        HttpEntity<Map<String, Object>> entity = new HttpEntity<>(request, headers);

        try {
            ResponseEntity<Map> response = restTemplate.exchange(
                url, HttpMethod.POST, entity, Map.class);
            return response.getBody();
        } catch (Exception e) {
            throw new RuntimeException("媒体分析失败", e);
        }
    }

    public Map<String, Object> batchAnalyze(List<Map<String, Object>> requests) {
        if (!ensureValidToken()) {
            throw new RuntimeException("令牌无效且刷新失败");
        }

        String url = baseUrl + "/api/batch_analyze";

        Map<String, Object> request = new HashMap<>();
        request.put("requests", requests);

        HttpHeaders headers = new HttpHeaders();
        headers.setBearerAuth(accessToken);
        headers.setContentType(MediaType.APPLICATION_JSON);

        HttpEntity<Map<String, Object>> entity = new HttpEntity<>(request, headers);

        try {
            ResponseEntity<Map> response = restTemplate.exchange(
                url, HttpMethod.POST, entity, Map.class);
            return response.getBody();
        } catch (Exception e) {
            throw new RuntimeException("批量媒体分析失败", e);
        }
    }

    public Map<String, Object> queryResults(String queryType, Map<String, Object> params) {
        if (!ensureValidToken()) {
            throw new RuntimeException("令牌无效且刷新失败");
        }

        String url = baseUrl + "/api/query";

        Map<String, Object> request = new HashMap<>();
        request.put("query_type", queryType);
        request.putAll(params);

        HttpHeaders headers = new HttpHeaders();
        headers.setBearerAuth(accessToken);
        headers.setContentType(MediaType.APPLICATION_JSON);

        HttpEntity<Map<String, Object>> entity = new HttpEntity<>(request, headers);

        try {
            ResponseEntity<Map> response = restTemplate.exchange(
                url, HttpMethod.POST, entity, Map.class);
            return response.getBody();
        } catch (Exception e) {
            throw new RuntimeException("查询失败", e);
        }
    }
}

// DoubaoAnalyzerController.java
@RestController
@RequestMapping("/api/media")
public class DoubaoAnalyzerController {
    private final DoubaoAnalyzerService analyzerService;

    @Autowired
    public DoubaoAnalyzerController(DoubaoAnalyzerService analyzerService) {
        this.analyzerService = analyzerService;
    }

    @PostMapping("/login")
    public ResponseEntity<Map<String, Object>> login(@RequestBody Map<String, String> credentials) {
        String username = credentials.get("username");
        String password = credentials.get("password");

        boolean success = analyzerService.login(username, password);

        Map<String, Object> response = new HashMap<>();
        response.put("success", success);

        if (success) {
            response.put("message", "登录成功");
        } else {
            response.put("message", "登录失败");
        }

        return ResponseEntity.ok(response);
    }

    @PostMapping("/analyze")
    public ResponseEntity<Map<String, Object>> analyzeMedia(@RequestBody Map<String, Object> request) {
        String mediaType = (String) request.get("media_type");
        String mediaUrl = (String) request.get("media_url");
        String prompt = (String) request.get("prompt");
        Integer maxTokens = (Integer) request.get("max_tokens");
        Integer videoFrames = (Integer) request.get("video_frames");
        Boolean saveToDb = (Boolean) request.get("save_to_db");

        try {
            Map<String, Object> result = analyzerService.analyzeMedia(
                mediaType, mediaUrl, prompt, maxTokens, videoFrames, saveToDb);
            return ResponseEntity.ok(result);
        } catch (Exception e) {
            Map<String, Object> errorResponse = new HashMap<>();
            errorResponse.put("success", false);
            errorResponse.put("message", "分析失败: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(errorResponse);
        }
    }

    @PostMapping("/batch-analyze")
    public ResponseEntity<Map<String, Object>> batchAnalyze(@RequestBody Map<String, Object> request) {
        @SuppressWarnings("unchecked")
        List<Map<String, Object>> mediaRequests = (List<Map<String, Object>>) request.get("requests");

        try {
            Map<String, Object> result = analyzerService.batchAnalyze(mediaRequests);
            return ResponseEntity.ok(result);
        } catch (Exception e) {
            Map<String, Object> errorResponse = new HashMap<>();
            errorResponse.put("success", false);
            errorResponse.put("message", "批量分析失败: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(errorResponse);
        }
    }

    @PostMapping("/query")
    public ResponseEntity<Map<String, Object>> queryResults(@RequestBody Map<String, Object> request) {
        String queryType = (String) request.get("query_type");

        // 移除query_type，将剩余参数传递给服务
        Map<String, Object> params = new HashMap<>(request);
        params.remove("query_type");

        try {
            Map<String, Object> result = analyzerService.queryResults(queryType, params);
            return ResponseEntity.ok(result);
        } catch (Exception e) {
            Map<String, Object> errorResponse = new HashMap<>();
            errorResponse.put("success", false);
            errorResponse.put("message", "查询失败: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(errorResponse);
        }
    }
}

// application.properties
doubao.api.url=http://your-server:8080/api
```

## 部署建议

### 生产环境部署

1. **负载均衡**:
   - 使用Nginx作为反向代理和负载均衡器
   - 配置多个API服务器实例

2. **缓存策略**:
   - 对频繁查询的结果使用Redis缓存
   - 实现智能缓存失效机制

3. **监控与告警**:
   - 集成Prometheus和Grafana监控系统性能
   - 设置关键指标告警

4. **安全加固**:
   - 使用HTTPS加密通信
   - 实现API访问频率限制
   - 配置防火墙规则

### 容器化部署

1. **Dockerfile示例**:

```dockerfile
FROM ubuntu:20.04

# 安装依赖
RUN apt-get update && apt-get install -y     build-essential cmake pkg-config     libopencv-dev libcurl4-openssl-dev     nlohmann-json3-dev libmysqlclient-dev     ffmpeg &&     rm -rf /var/lib/apt/lists/*

# 复制源代码
COPY . /app
WORKDIR /app

# 编译
RUN mkdir build && cd build &&     cmake .. &&     make -j$(nproc) &&     make install

# 创建运行用户
RUN useradd -m -s /bin/bash doubao &&     mkdir -p /home/doubao/.doubao_analyzer &&     chown -R doubao:doubao /home/doubao

# 复制配置文件
COPY config/docker-config.json /home/doubao/.doubao_analyzer/config.json
RUN chown doubao:doubao /home/doubao/.doubao_analyzer/config.json

# 暴露端口
EXPOSE 8080

# 切换到运行用户
USER doubao

# 启动命令
CMD ["doubao_api_server"]
```

2. **Docker Compose示例**:

```yaml
version: '3.8'

services:
  doubao-api:
    build: .
    ports:
      - "8080:8080"
    environment:
      - DOUBAO_API_KEY=${DOUBAO_API_KEY}
    volumes:
      - ./config:/home/doubao/.doubao_analyzer
    depends_on:
      - mysql
    restart: unless-stopped

  mysql:
    image: mysql:5.7
    environment:
      MYSQL_ROOT_PASSWORD: ${MYSQL_ROOT_PASSWORD}
      MYSQL_DATABASE: doubao_analyzer
      MYSQL_USER: doubao_user
      MYSQL_PASSWORD: ${MYSQL_PASSWORD}
    volumes:
      - mysql_data:/var/lib/mysql
      - ./scripts/init_database.sql:/docker-entrypoint-initdb.d/init.sql
    restart: unless-stopped

  redis:
    image: redis:6-alpine
    restart: unless-stopped

  nginx:
    image: nginx:alpine
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx/nginx.conf:/etc/nginx/nginx.conf
      - ./nginx/ssl:/etc/nginx/ssl
    depends_on:
      - doubao-api
    restart: unless-stopped

volumes:
  mysql_data:
```

### Kubernetes部署

1. **部署清单示例**:

```yaml
# doubao-api-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: doubao-api
  labels:
    app: doubao-api
spec:
  replicas: 3
  selector:
    matchLabels:
      app: doubao-api
  template:
    metadata:
      labels:
        app: doubao-api
    spec:
      containers:
      - name: doubao-api
        image: your-registry/doubao-api:latest
        ports:
        - containerPort: 8080
        env:
        - name: DOUBAO_API_KEY
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: api-key
        - name: DB_HOST
          value: mysql-service
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: db-user
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: db-password
        volumeMounts:
        - name: config-volume
          mountPath: /home/doubao/.doubao_analyzer
        resources:
          requests:
            memory: "1Gi"
            cpu: "500m"
          limits:
            memory: "2Gi"
            cpu: "1000m"
        livenessProbe:
          httpGet:
            path: /api/status
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/status
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
      volumes:
      - name: config-volume
        configMap:
          name: doubao-config

---
# doubao-api-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: doubao-api-service
spec:
  selector:
    app: doubao-api
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: ClusterIP

---
# doubao-api-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: doubao-api-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
    cert-manager.io/cluster-issuer: letsencrypt-prod
spec:
  tls:
  - hosts:
    - api.doubao.example.com
    secretName: doubao-api-tls
  rules:
  - host: api.doubao.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: doubao-api-service
            port:
              number: 80
```

## 总结

豆包媒体分析系统提供了强大的图片和视频分析功能，通过RESTful API接口可以轻松集成到各种应用中。本文档详细介绍了API的使用方法、技术实现细节和集成示例，帮助开发者快速构建基于豆包媒体分析功能的应用。

系统的核心优势包括：
- 基于豆包大模型的智能分析能力
- 完善的JWT认证和授权机制
- 灵活的查询和批量处理功能
- 良好的扩展性和可维护性

通过遵循本文档的指南，开发者可以高效地将豆包媒体分析功能集成到自己的应用中，为用户提供智能的媒体内容分析服务。
