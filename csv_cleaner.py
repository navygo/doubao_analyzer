import sys
import pandas as pd
import re

def clean_value(value):
    if isinstance(value, str):
        # 移除可能导致问题的字符
        # 保留中文、英文、数字和基本标点
        return re.sub(r'[^-一-鿿　-〿＀-￯]', '', value)
    return value

try:
    # 读取CSV文件
    df = pd.read_csv(sys.argv[1], encoding='utf-8-sig')

    # 清理所有字符串值
    for col in df.columns:
        if df[col].dtype == 'object':
            df[col] = df[col].apply(clean_value)

    # 写入XLSX文件
    df.to_excel(sys.argv[2], index=False, engine='openpyxl')
    print('转换成功')
except Exception as e:
    print(f'转换错误: {e}')
    sys.exit(1)
