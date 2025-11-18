        <div class="tabs">
            <button class="tab active" data-tab="analyze">媒体分析</button>
            <button class="tab" data-tab="query">结果查询</button>
            <button class="tab" data-tab="status">系统状态</button>
        </div>

        <!-- 媒体分析选项卡 -->
        <div class="tab-content active" id="analyzeTab">
            <div class="section">
                <h2>媒体分析</h2>

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

                <div class="form-group">
                    <label for="prompt">自定义提示词 (可选):</label>
                    <textarea id="prompt" rows="3" placeholder="留空则使用默认提示词"></textarea>
                </div>

                <div class="form-group">
                    <label for="maxTokens">最大令牌数:</label>
                    <input type="number" id="maxTokens" value="1500">
                </div>

                <div class="form-group" id="videoFramesGroup">
                    <label for="videoFrames">视频帧数:</label>
                    <input type="number" id="videoFrames" value="5">
                </div>

                <div class="form-group">
                    <label>
                        <input type="checkbox" id="saveToDb" checked> 保存结果到数据库
                    </label>
                </div>

                <button id="analyzeBtn">分析</button>
                <div id="analyzeResult" class="result"></div>
            </div>

            <!-- 批量分析部分 -->
            <div class="section">
                <h2>批量分析</h2>

                <div class="form-group">
                    <label for="batchUrls">媒体URL列表 (每行一个):</label>
                    <textarea id="batchUrls" rows="5" placeholder="https://example.com/image1.jpg&#10;https://example.com/video1.mp4"></textarea>
                </div>

                <button id="batchAnalyzeBtn">批量分析</button>
                <div id="batchAnalyzeResult" class="result"></div>
            </div>
        </div>

        <!-- 结果查询选项卡 -->
        <div class="tab-content" id="queryTab">
            <div class="section">
                <h2>查询分析结果</h2>

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

                <div class="form-group" id="tagGroup">
                    <label for="tag">标签:</label>
                    <input type="text" id="tag" placeholder="风景">
                </div>

                <div class="form-group" id="fileTypeGroup">
                    <label for="fileType">文件类型:</label>
                    <select id="fileType">
                        <option value="image">图片</option>
                        <option value="video">视频</option>
                    </select>
                </div>

                <div class="form-group" id="dateRangeGroup">
                    <label for="startDate">开始日期:</label>
                    <input type="date" id="startDate">

                    <label for="endDate">结束日期:</label>
                    <input type="date" id="endDate">
                </div>

                <div class="form-group" id="limitGroup">
                    <label for="limit">记录数量限制:</label>
                    <input type="number" id="limit" value="10">
                </div>

                <div class="form-group" id="conditionGroup">
                    <label for="condition">自定义查询条件:</label>
                    <textarea id="condition" rows="3" placeholder="例如: file_type='image' AND created_at > '2023-07-01'"></textarea>
                </div>

                <div class="form-group" id="mediaUrlGroup">
                    <label for="queryMediaUrl">媒体URL:</label>
                    <input type="text" id="queryMediaUrl" placeholder="https://example.com/image.jpg">
                </div>

                <button id="queryBtn">查询</button>
                <div id="queryResult" class="result"></div>
            </div>
        </div>

        <!-- 系统状态选项卡 -->
        <div class="tab-content" id="statusTab">
            <div class="section">
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

        // DOM元素
        const loginSection = document.getElementById('loginSection');
        const loginBtn = document.getElementById('loginBtn');
        const loginResult = document.getElementById('loginResult');

        const tabs = document.querySelectorAll('.tab');
        const tabContents = document.querySelectorAll('.tab-content');

        const mediaType = document.getElementById('mediaType');
        const videoFramesGroup = document.getElementById('videoFramesGroup');

        const queryType = document.getElementById('queryType');
        const tagGroup = document.getElementById('tagGroup');
        const fileTypeGroup = document.getElementById('fileTypeGroup');
        const dateRangeGroup = document.getElementById('dateRangeGroup');
        const limitGroup = document.getElementById('limitGroup');
        const conditionGroup = document.getElementById('conditionGroup');
        const mediaUrlGroup = document.getElementById('mediaUrlGroup');

        // API服务器地址 (根据实际情况修改)
        const API_BASE_URL = 'http://localhost:8080';

        // 初始化
        document.addEventListener('DOMContentLoaded', () => {
            // 选项卡切换
            tabs.forEach(tab => {
                tab.addEventListener('click', () => {
                    const tabName = tab.getAttribute('data-tab');

                    // 更新选项卡样式
                    tabs.forEach(t => t.classList.remove('active'));
                    tab.classList.add('active');

                    // 显示对应内容
                    tabContents.forEach(content => {
                        content.classList.remove('active');
                        if (content.id === `${tabName}Tab`) {
                            content.classList.add('active');
                        }
                    });

                    // 根据查询类型显示相应输入框
                    updateQueryFields();
                });
            });

            // 媒体类型切换
            mediaType.addEventListener('change', () => {
                if (mediaType.value === 'video') {
                    videoFramesGroup.style.display = 'block';
                } else {
                    videoFramesGroup.style.display = 'none';
                }
            });

            // 查询类型切换
            queryType.addEventListener('change', updateQueryFields);

            // 登录按钮点击事件
            loginBtn.addEventListener('click', login);

            // 分析按钮点击事件
            document.getElementById('analyzeBtn').addEventListener('click', analyzeMedia);

            // 批量分析按钮点击事件
            document.getElementById('batchAnalyzeBtn').addEventListener('click', batchAnalyze);

            // 查询按钮点击事件
            document.getElementById('queryBtn').addEventListener('click', queryResults);

            // 状态按钮点击事件
            document.getElementById('statusBtn').addEventListener('click', getServerStatus);

            // 初始化界面
            updateQueryFields();
        });

        // 更新查询字段显示
        function updateQueryFields() {
            // 隐藏所有字段组
            tagGroup.style.display = 'none';
            fileTypeGroup.style.display = 'none';
            dateRangeGroup.style.display = 'none';
            limitGroup.style.display = 'none';
            conditionGroup.style.display = 'none';
            mediaUrlGroup.style.display = 'none';

            // 根据查询类型显示相应字段
            switch (queryType.value) {
                case 'tag':
                    tagGroup.style.display = 'block';
                    break;
                case 'type':
                    fileTypeGroup.style.display = 'block';
                    break;
                case 'date_range':
                    dateRangeGroup.style.display = 'block';
                    break;
                case 'recent':
                    limitGroup.style.display = 'block';
                    break;
                case 'url':
                    mediaUrlGroup.style.display = 'block';
                    break;
                case 'all':
                    conditionGroup.style.display = 'block';
                    break;
            }
        }

        // 登录函数
        async function login() {
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;

            try {
                const response = await fetch(`${API_BASE_URL}/api/auth`, {
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

                    loginResult.innerHTML = `<div class="success">登录成功！</div>`;

                    // 隐藏登录部分，显示功能部分
                    setTimeout(() => {
                        loginSection.style.display = 'none';
                    }, 1000);
                } else {
                    loginResult.innerHTML = `<div class="error">登录失败: ${result.message}</div>`;
                }
            } catch (error) {
                loginResult.innerHTML = `<div class="error">登录错误: ${error.message}</div>`;
            }
        }

        // 确保令牌有效
        async function ensureValidToken() {
            if (Date.now() >= tokenExpiresAt) {
                try {
                    const response = await fetch(`${API_BASE_URL}/api/auth/refresh`, {
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
                        return true;
                    }
                    return false;
                } catch (error) {
                    console.error('Token refresh error:', error);
                    return false;
                }
            }
            return true;
        }

        // 分析媒体
        async function analyzeMedia() {
            if (!(await ensureValidToken())) {
                alert('令牌已失效，请重新登录');
                loginSection.style.display = 'block';
                return;
            }

            const mediaTypeValue = mediaType.value;
            const mediaUrl = document.getElementById('mediaUrl').value;
            const prompt = document.getElementById('prompt').value;
            const maxTokens = parseInt(document.getElementById('maxTokens').value);
            const videoFrames = parseInt(document.getElementById('videoFrames').value);
            const saveToDb = document.getElementById('saveToDb').checked;

            const analyzeResult = document.getElementById('analyzeResult');
            analyzeResult.innerHTML = '分析中，请稍候...';

            try {
                const data = {
                    media_type: mediaTypeValue,
                    media_url: mediaUrl,
                    save_to_db: saveToDb
                };

                if (prompt) data.prompt = prompt;
                if (maxTokens) data.max_tokens = maxTokens;
                if (mediaTypeValue === 'video' && videoFrames) data.video_frames = videoFrames;

                const response = await fetch(`${API_BASE_URL}/api/analyze`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    analyzeResult.innerHTML = `
                        <div class="success">${result.message}</div>
                        <div>响应时间: ${result.response_time}秒</div>
                        <div>分析内容: ${result.data.content}</div>
                        <div>提取标签: ${JSON.stringify(result.data.tags)}</div>
                        <div>令牌使用: ${JSON.stringify(result.data.usage)}</div>
                        <div>已保存到数据库: ${result.data.saved_to_db ? '是' : '否'}</div>
                    `;
                } else {
                    analyzeResult.innerHTML = `<div class="error">分析失败: ${result.message}</div>`;
                }
            } catch (error) {
                analyzeResult.innerHTML = `<div class="error">分析错误: ${error.message}</div>`;
            }
        }

        // 批量分析
        async function batchAnalyze() {
            if (!(await ensureValidToken())) {
                alert('令牌已失效，请重新登录');
                loginSection.style.display = 'block';
                return;
            }

            const batchUrlsText = document.getElementById('batchUrls').value;
            const urls = batchUrlsText.split('
').filter(url => url.trim());

            if (urls.length === 0) {
                alert('请输入至少一个URL');
                return;
            }

            const batchAnalyzeResult = document.getElementById('batchAnalyzeResult');
            batchAnalyzeResult.innerHTML = '批量分析中，请稍候...';

            try {
                const requests = urls.map(url => {
                    // 简单判断URL是图片还是视频
                    const isVideo = /\.(mp4|avi|mov|mkv|wmv|flv|webm)$/i.test(url);

                    return {
                        media_type: isVideo ? 'video' : 'image',
                        media_url: url.trim(),
                        save_to_db: true
                    };
                });

                const response = await fetch(`${API_BASE_URL}/api/batch_analyze`, {
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
                    let html = `
                        <div class="success">${result.message}</div>
                        <div>总响应时间: ${result.response_time}秒</div>
                        <div>处理统计: ${JSON.stringify(result.data.summary)}</div>
                        <div>详细结果:</div>
                        <ul>
                    `;

                    result.data.results.forEach(item => {
                        if (item.success) {
                            html += `<li>成功: ${item.task_id} - 标签: ${JSON.stringify(item.tags)}</li>`;
                        } else {
                            html += `<li class="error">失败: ${item.task_id} - 错误: ${item.error}</li>`;
                        }
                    });

                    html += '</ul>';
                    batchAnalyzeResult.innerHTML = html;
                } else {
                    batchAnalyzeResult.innerHTML = `<div class="error">批量分析失败: ${result.message}</div>`;
                }
            } catch (error) {
                batchAnalyzeResult.innerHTML = `<div class="error">批量分析错误: ${error.message}</div>`;
            }
        }

        // 查询结果
        async function queryResults() {
            if (!(await ensureValidToken())) {
                alert('令牌已失效，请重新登录');
                loginSection.style.display = 'block';
                return;
            }

            const queryTypeValue = queryType.value;
            const queryResult = document.getElementById('queryResult');
            queryResult.innerHTML = '查询中，请稍候...';

            try {
                const data = {
                    query_type: queryTypeValue
                };

                // 根据查询类型添加相应参数
                switch (queryTypeValue) {
                    case 'tag':
                        data.tag = document.getElementById('tag').value;
                        break;
                    case 'type':
                        data.file_type = document.getElementById('fileType').value;
                        break;
                    case 'date_range':
                        data.start_date = document.getElementById('startDate').value;
                        data.end_date = document.getElementById('endDate').value;
                        break;
                    case 'recent':
                        data.limit = parseInt(document.getElementById('limit').value);
                        break;
                    case 'url':
                        data.media_url = document.getElementById('queryMediaUrl').value;
                        break;
                    case 'all':
                        data.condition = document.getElementById('condition').value;
                        break;
                }

                const response = await fetch(`${API_BASE_URL}/api/query`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${accessToken}`,
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (result.success) {
                    let html = `
                        <div class="success">${result.message}</div>
                        <div>响应时间: ${result.response_time}秒</div>
                        <div>查询结果 (${result.data.count}条):</div>
                        <ul>
                    `;

                    result.data.results.forEach(item => {
                        html += `
                            <li>
                                <strong>ID: ${item.id}</strong> - ${item.file_name} (${item.file_type})
                                <br>路径: ${item.file_path}
                                <br>标签: ${item.tags}
                                <br>创建时间: ${item.created_at}
                                <br>分析结果: ${item.analysis_result.substring(0, 100)}${item.analysis_result.length > 100 ? '...' : ''}
                            </li>
                        `;
                    });

                    html += '</ul>';
                    queryResult.innerHTML = html;
                } else {
                    queryResult.innerHTML = `<div class="error">查询失败: ${result.message}</div>`;
                }
            } catch (error) {
                queryResult.innerHTML = `<div class="error">查询错误: ${error.message}</div>`;
            }
        }

        // 获取服务器状态
        async function getServerStatus() {
            const statusResult = document.getElementById('statusResult');
            statusResult.innerHTML = '获取状态中，请稍候...';

            try {
                const response = await fetch(`${API_BASE_URL}/api/status`);
                const result = await response.json();

                if (result.success) {
                    statusResult.innerHTML = `
                        <div class="success">${result.message}</div>
                        <div>服务器状态: ${result.data.server_status}</div>
                        <div>监听地址: ${result.data.host}:${result.data.port}</div>
                        <div>API密钥已设置: ${result.data.api_key_set ? '是' : '否'}</div>
                        <div>数据库统计: ${JSON.stringify(result.data.database_stats)}</div>
                    `;
                } else {
                    statusResult.innerHTML = `<div class="error">获取状态失败: ${result.message}</div>`;
                }
            } catch (error) {
                statusResult.innerHTML = `<div class="error">获取状态错误: ${error.message}</div>`;
            }
        }
    </script>
</body>
</html>
```

### Java SDK示例

```java
import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.time.Instant;
import java.util.Base64;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

public class DoubaoAnalyzerClient {
    private static final Duration TIMEOUT = Duration.ofSeconds(30);

    private final String baseUrl;
    private final HttpClient httpClient;
    private final ObjectMapper objectMapper;

    private String accessToken;
    private String refreshToken;
    private Instant tokenExpiresAt;

    public DoubaoAnalyzerClient(String baseUrl) {
        this.baseUrl = baseUrl;
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(TIMEOUT)
                .build();
        this.objectMapper = new ObjectMapper();
    }

    /**
     * 登录获取访问令牌和刷新令牌
     * @param username 用户名
     * @param password 密码
     * @return 是否登录成功
     */
    public boolean login(String username, String password) {
        try {
            Map<String, String> requestBody = new HashMap<>();
            requestBody.put("username", username);
            requestBody.put("password", password);

            String jsonBody = objectMapper.writeValueAsString(requestBody);

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(baseUrl + "/api/auth"))
                    .header("Content-Type", "application/json")
                    .timeout(TIMEOUT)
                    .POST(HttpRequest.BodyPublishers.ofString(jsonBody))
                    .build();

            HttpResponse<String> response = httpClient.send(request, 
                    HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() == 200) {
                AuthResponse authResponse = objectMapper.readValue(response.body(), AuthResponse.class);

                if (authResponse.isSuccess()) {
                    this.accessToken = authResponse.getData().getAccessToken();
                    this.refreshToken = authResponse.getData().getRefreshToken();
                    // 提前1分钟过期
                    this.tokenExpiresAt = Instant.now().plusSeconds(authResponse.getData().getExpiresIn() - 60);
                    return true;
                }
            }

            return false;
        } catch (Exception e) {
            System.err.println("Login error: " + e.getMessage());
            return false;
        }
    }

    /**
     * 刷新访问令牌
     * @return 是否刷新成功
     */
    public boolean refreshAccessToken() {
        try {
            Map<String, String> requestBody = new HashMap<>();
            requestBody.put("refresh_token", refreshToken);

            String jsonBody = objectMapper.writeValueAsString(requestBody);

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(baseUrl + "/api/auth/refresh"))
                    .header("Content-Type", "application/json")
                    .timeout(TIMEOUT)
                    .POST(HttpRequest.BodyPublishers.ofString(jsonBody))
                    .build();

            HttpResponse<String> response = httpClient.send(request, 
                    HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() == 200) {
                AuthResponse authResponse = objectMapper.readValue(response.body(), AuthResponse.class);

                if (authResponse.isSuccess()) {
                    this.accessToken = authResponse.getData().getAccessToken();
                    this.refreshToken = authResponse.getData().getRefreshToken();
                    // 提前1分钟过期
                    this.tokenExpiresAt = Instant.now().plusSeconds(authResponse.getData().getExpiresIn() - 60);
                    return true;
                }
            }

            return false;
        } catch (Exception e) {
            System.err.println("Token refresh error: " + e.getMessage());
            return false;
        }
    }

    /**
     * 确保令牌有效
     * @return 是否令牌有效
     */
    public boolean ensureValidToken() {
        if (Instant.now().isAfter(tokenExpiresAt)) {
            return refreshAccessToken();
        }
        return true;
    }

    /**
     * 分析单个媒体文件
     * @param mediaType 媒体类型 ("image" 或 "video")
     * @param mediaUrl 媒体URL
     * @param prompt 自定义提示词 (可选)
     * @param maxTokens 最大令牌数 (可选)
     * @param videoFrames 视频帧数 (仅视频分析有效，可选)
     * @param saveToDb 是否保存到数据库
     * @return 分析结果
     */
    public ApiResponse analyzeMedia(String mediaType, String mediaUrl, 
                                  String prompt, Integer maxTokens, 
                                  Integer videoFrames, Boolean saveToDb) {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        try {
            Map<String, Object> requestBody = new HashMap<>();
            requestBody.put("media_type", mediaType);
            requestBody.put("media_url", mediaUrl);
            requestBody.put("save_to_db", saveToDb != null ? saveToDb : true);

            if (prompt != null) requestBody.put("prompt", prompt);
            if (maxTokens != null) requestBody.put("max_tokens", maxTokens);
            if (videoFrames != null && "video".equals(mediaType)) {
                requestBody.put("video_frames", videoFrames);
            }

            String jsonBody = objectMapper.writeValueAsString(requestBody);

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(baseUrl + "/api/analyze"))
                    .header("Authorization", "Bearer " + accessToken)
                    .header("Content-Type", "application/json")
                    .timeout(TIMEOUT)
                    .POST(HttpRequest.BodyPublishers.ofString(jsonBody))
                    .build();

            HttpResponse<String> response = httpClient.send(request, 
                    HttpResponse.BodyHandlers.ofString());

            return objectMapper.readValue(response.body(), ApiResponse.class);
        } catch (Exception e) {
            throw new RuntimeException("Analysis error: " + e.getMessage(), e);
        }
    }

    /**
     * 批量分析媒体文件
     * @param requests 分析请求列表
     * @return 批量分析结果
     */
    public ApiResponse batchAnalyze(List<AnalyzeRequest> requests) {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        try {
            Map<String, Object> requestBody = new HashMap<>();
            requestBody.put("requests", requests);

            String jsonBody = objectMapper.writeValueAsString(requestBody);

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(baseUrl + "/api/batch_analyze"))
                    .header("Authorization", "Bearer " + accessToken)
                    .header("Content-Type", "application/json")
                    .timeout(TIMEOUT)
                    .POST(HttpRequest.BodyPublishers.ofString(jsonBody))
                    .build();

            HttpResponse<String> response = httpClient.send(request, 
                    HttpResponse.BodyHandlers.ofString());

            return objectMapper.readValue(response.body(), ApiResponse.class);
        } catch (Exception e) {
            throw new RuntimeException("Batch analysis error: " + e.getMessage(), e);
        }
    }

    /**
     * 查询分析结果
     * @param queryType 查询类型
     * @param params 查询参数
     * @return 查询结果
     */
    public ApiResponse queryResults(String queryType, Map<String, Object> params) {
        if (!ensureValidToken()) {
            throw new RuntimeException("Failed to refresh token");
        }

        try {
            Map<String, Object> requestBody = new HashMap<>();
            requestBody.put("query_type", queryType);

            if (params != null) {
                requestBody.putAll(params);
            }

            String jsonBody = objectMapper.writeValueAsString(requestBody);

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(baseUrl + "/api/query"))
                    .header("Authorization", "Bearer " + accessToken)
                    .header("Content-Type", "application/json")
                    .timeout(TIMEOUT)
                    .POST(HttpRequest.BodyPublishers.ofString(jsonBody))
                    .build();

            HttpResponse<String> response = httpClient.send(request, 
                    HttpResponse.BodyHandlers.ofString());

            return objectMapper.readValue(response.body(), ApiResponse.class);
        } catch (Exception e) {
            throw new RuntimeException("Query error: " + e.getMessage(), e);
        }
    }

    /**
     * 获取服务器状态
     * @return 服务器状态
     */
    public ApiResponse getServerStatus() {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(baseUrl + "/api/status"))
                    .timeout(TIMEOUT)
                    .GET()
                    .build();

            HttpResponse<String> response = httpClient.send(request, 
                    HttpResponse.BodyHandlers.ofString());

            return objectMapper.readValue(response.body(), ApiResponse.class);
        } catch (Exception e) {
            throw new RuntimeException("Status query error: " + e.getMessage(), e);
        }
    }

    // 内部类定义

    public static class AuthResponse {
        private boolean success;
        private String message;
        private AuthData data;

        // Getters and setters
        public boolean isSuccess() { return success; }
        public void setSuccess(boolean success) { this.success = success; }

        public String getMessage() { return message; }
        public void setMessage(String message) { this.message = message; }

        public AuthData getData() { return data; }
        public void setData(AuthData data) { this.data = data; }
    }

    public static class AuthData {
        @JsonProperty("access_token")
        private String accessToken;

        @JsonProperty("expires_in")
        private int expiresIn;

        @JsonProperty("refresh_token")
        private String refreshToken;

        @JsonProperty("refresh_expires_in")
        private int refreshExpiresIn;

        // Getters and setters
        public String getAccessToken() { return accessToken; }
        public void setAccessToken(String accessToken) { this.accessToken = accessToken; }

        public int getExpiresIn() { return expiresIn; }
        public void setExpiresIn(int expiresIn) { this.expiresIn = expiresIn; }

        public String getRefreshToken() { return refreshToken; }
        public void setRefreshToken(String refreshToken) { this.refreshToken = refreshToken; }

        public int getRefreshExpiresIn() { return refreshExpiresIn; }
        public void setRefreshExpiresIn(int refreshExpiresIn) { this.refreshExpiresIn = refreshExpiresIn; }
    }

    public static class ApiResponse {
        private boolean success;
        private String message;
        private Map<String, Object> data;
        private double responseTime;
        private String error;

        // Getters and setters
        public boolean isSuccess() { return success; }
        public void setSuccess(boolean success) { this.success = success; }

        public String getMessage() { return message; }
        public void setMessage(String message) { this.message = message; }

        public Map<String, Object> getData() { return data; }
        public void setData(Map<String, Object> data) { this.data = data; }

        public double getResponseTime() { return responseTime; }
        public void setResponseTime(double responseTime) { this.responseTime = responseTime; }

        public String getError() { return error; }
        public void setError(String error) { this.error = error; }
    }

    public static class AnalyzeRequest {
        @JsonProperty("media_type")
        private String mediaType;

        @JsonProperty("media_url")
        private String mediaUrl;

        private String prompt;

        @JsonProperty("max_tokens")
        private Integer maxTokens;

        @JsonProperty("video_frames")
        private Integer videoFrames;

        @JsonProperty("save_to_db")
        private Boolean saveToDb;

        // Getters and setters
        public String getMediaType() { return mediaType; }
        public void setMediaType(String mediaType) { this.mediaType = mediaType; }

        public String getMediaUrl() { return mediaUrl; }
        public void setMediaUrl(String mediaUrl) { this.mediaUrl = mediaUrl; }

        public String getPrompt() { return prompt; }
        public void setPrompt(String prompt) { this.prompt = prompt; }

        public Integer getMaxTokens() { return maxTokens; }
        public void setMaxTokens(Integer maxTokens) { this.maxTokens = maxTokens; }

        public Integer getVideoFrames() { return videoFrames; }
        public void setVideoFrames(Integer videoFrames) { this.videoFrames = videoFrames; }

        public Boolean getSaveToDb() { return saveToDb; }
        public void setSaveToDb(Boolean saveToDb) { this.saveToDb = saveToDb; }
    }

    // 使用示例
    public static void main(String[] args) {
        // 初始化客户端
        DoubaoAnalyzerClient client = new DoubaoAnalyzerClient("http://your-server:8080");

        // 登录
        boolean loginSuccess = client.login("admin", "admin123");
        if (!loginSuccess) {
            System.err.println("登录失败");
            return;
        }

        try {
            // 分析单张图片
            ApiResponse imageResult = client.analyzeMedia(
                "image", 
                "https://example.com/image.jpg", 
                null, 
                null, 
                null, 
                true
            );
            System.out.println("图片分析结果: " + imageResult.getMessage());

            // 分析单个视频
            ApiResponse videoResult = client.analyzeMedia(
                "video", 
                "https://example.com/video.mp4", 
                null, 
                null, 
                8, 
                true
            );
            System.out.println("视频分析结果: " + videoResult.getMessage());

            // 批量分析
            List<AnalyzeRequest> requests = List.of(
                createAnalyzeRequest("image", "https://example.com/image1.jpg"),
                createAnalyzeRequest("video", "https://example.com/video1.mp4", 5)
            );
            ApiResponse batchResult = client.batchAnalyze(requests);
            System.out.println("批量分析结果: " + batchResult.getMessage());

            // 查询结果
            Map<String, Object> queryParams = new HashMap<>();
            queryParams.put("tag", "风景");
            ApiResponse queryResult = client.queryResults("tag", queryParams);
            System.out.println("查询结果: " + queryResult.getMessage());

            // 获取服务器状态
            ApiResponse status = client.getServerStatus();
            System.out.println("服务器状态: " + status.getMessage());
        } catch (Exception e) {
            System.err.println("操作失败: " + e.getMessage());
        }
    }

    private static AnalyzeRequest createAnalyzeRequest(String mediaType, String mediaUrl) {
        return createAnalyzeRequest(mediaType, mediaUrl, null);
    }

    private static AnalyzeRequest createAnalyzeRequest(String mediaType, String mediaUrl, Integer videoFrames) {
        AnalyzeRequest request = new AnalyzeRequest();
        request.setMediaType(mediaType);
        request.setMediaUrl(mediaUrl);
        request.setSaveToDb(true);
        if (videoFrames != null) {
            request.setVideoFrames(videoFrames);
        }
        return request;
    }
}
```

## 部署架构建议

### 生产环境架构

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   前端应用      │────▶│   API网关       │────▶│  负载均衡器     │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                                                       │
                              ┌───────────────────────┼───────────────────────┐
                              │                       │                       │
                    ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
                    │ API服务器实例1   │     │ API服务器实例2   │     │ API服务器实例N   │
                    └─────────────────┘     └─────────────────┘     └─────────────────┘
                              │                       │                       │
                              └───────────────────────┼───────────────────────┘
                                                      │
                                              ┌─────────────────┐
                                              │   MySQL数据库    │
                                              └─────────────────┘
```

### 容器化部署

#### Dockerfile示例

```dockerfile
FROM ubuntu:20.04

# 设置非交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 安装依赖
RUN apt-get update && apt-get install -y     build-essential     cmake     pkg-config     libopencv-dev     libcurl4-openssl-dev     nlohmann-json3-dev     libmysqlclient-dev     ffmpeg     && rm -rf /var/lib/apt/lists/*

# 复制源代码
COPY . /app
WORKDIR /app

# 编译项目
RUN mkdir build && cd build &&     cmake .. &&     make -j$(nproc) &&     make install

# 创建非root用户
RUN useradd -m -u 1000 doubao &&     mkdir -p /home/doubao/.doubao_analyzer &&     chown -R doubao:doubao /home/doubao

# 切换到非root用户
USER doubao

# 暴露端口
EXPOSE 8080

# 启动命令
CMD ["doubao_api_server", "--api-key", "${API_KEY}", "--port", "8080"]
```

#### Docker Compose示例

```yaml
version: '3.8'

services:
  doubao-api:
    build: .
    ports:
      - "8080:8080"
    environment:
      - API_KEY=${API_KEY}
    volumes:
      - ./config:/home/doubao/.doubao_analyzer
    depends_on:
      - mysql
    restart: unless-stopped

  mysql:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: ${MYSQL_ROOT_PASSWORD}
      MYSQL_DATABASE: doubao_analyzer
      MYSQL_USER: doubao_user
      MYSQL_PASSWORD: ${MYSQL_PASSWORD}
    volumes:
      - mysql_data:/var/lib/mysql
      - ./scripts/init_database.sql:/docker-entrypoint-initdb.d/init.sql
    restart: unless-stopped

volumes:
  mysql_data:
```

### Kubernetes部署

#### Deployment配置示例

```yaml
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
        - name: API_KEY
          valueFrom:
            secretKeyRef:
              name: doubao-secrets
              key: api-key
        - name: DB_HOST
          value: "mysql-service"
        - name: DB_USER
          value: "doubao_user"
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
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
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
  type: LoadBalancer
---
apiVersion: v1
kind: Secret
metadata:
  name: doubao-secrets
type: Opaque
data:
  api-key: <base64-encoded-api-key>
  db-password: <base64-encoded-db-password>
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: doubao-config
data:
  config.json: |
    {
      "database": {
        "host": "mysql-service",
        "user": "doubao_user",
        "password": "",
        "database": "doubao_analyzer",
        "port": 3306,
        "charset": "utf8mb4",
        "connection_timeout": 60,
        "read_timeout": 60,
        "write_timeout": 60
      }
    }
```

## 总结

本技术文档详细介绍了豆包媒体分析系统的API接口、认证机制、错误处理、性能优化等方面，并提供了多种语言的SDK示例和集成指南。通过本文档，开发人员可以快速理解和使用API，实现与豆包媒体分析系统的集成。

对于生产环境部署，建议采用容器化部署方式，并结合负载均衡和高可用数据库集群，以确保系统的稳定性和可扩展性。

如有任何问题或建议，请联系技术支持团队。
