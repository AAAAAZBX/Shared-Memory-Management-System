#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
代码行数统计工具
统计项目中所有 .cpp 和 .h 文件的行数
"""

import os
from pathlib import Path
from collections import defaultdict

def count_lines(file_path):
    """统计文件行数"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            return len(f.readlines())
    except Exception as e:
        print(f"读取文件 {file_path} 时出错: {e}")
        return 0

def main():
    print("=" * 50)
    print("   Code Line Statistics")
    print("=" * 50)
    print()
    
    project_root = Path(__file__).parent
    cpp_files = []
    h_files = []
    dir_stats = defaultdict(int)
    
    # Count .cpp files
    print("C++ Source Files (.cpp):")
    print("-" * 50)
    
    for cpp_file in project_root.rglob("*.cpp"):
        if cpp_file.is_file():
            lines = count_lines(cpp_file)
            rel_path = cpp_file.relative_to(project_root)
            cpp_files.append((str(rel_path), lines))
            dir_stats[cpp_file.parent.relative_to(project_root)] += lines
            print(f"{str(rel_path):<50} {lines:>6} lines")
    
    cpp_total = sum(lines for _, lines in cpp_files)
    print("-" * 50)
    print(f"{'C++ Source Files Total:':<50} {cpp_total:>6} lines")
    print()
    
    # Count .h files
    print("C++ Header Files (.h):")
    print("-" * 50)
    
    for h_file in project_root.rglob("*.h"):
        if h_file.is_file():
            lines = count_lines(h_file)
            rel_path = h_file.relative_to(project_root)
            h_files.append((str(rel_path), lines))
            dir_stats[h_file.parent.relative_to(project_root)] += lines
            print(f"{str(rel_path):<50} {lines:>6} lines")
    
    h_total = sum(lines for _, lines in h_files)
    print("-" * 50)
    print(f"{'C++ Header Files Total:':<50} {h_total:>6} lines")
    print()
    
    # Statistics by directory
    print("Statistics by Directory:")
    print("-" * 50)
    
    for dir_path in sorted(dir_stats.keys()):
        print(f"{str(dir_path):<50} {dir_stats[dir_path]:>6} lines")
    
    print("-" * 50)
    print()
    
    # Total
    total = cpp_total + h_total
    print("=" * 50)
    print(f"Total Lines: {total} lines")
    print(f"  - C++ Source Files: {cpp_total} lines")
    print(f"  - C++ Header Files: {h_total} lines")
    print("=" * 50)

if __name__ == "__main__":
    main()
