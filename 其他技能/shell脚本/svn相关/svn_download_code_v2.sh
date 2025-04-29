#!/bin/bash

SVN_CODE_REPO_URL=
SVN_CODE_REPO_LOCAL_NAME=
SVN_CODE_REPO_USERNAME=
SVN_CODE_REPO_PASSWORD=
SVN_DOWNLOAD_CODE_PARA=
INITIAL_TIMEOUT=30            # 初始超时时间（秒）
MAX_TIMEOUT=300               # 最大超时时间（秒）
INITIAL_RETRY_DELAY=1         # 初始重试等待时间
MAX_RETRY_DELAY=30            # 最大重试间隔
MONITOR_INTERVAL=5            # 活动检测间隔（秒）

usage_info() {
    echo "--------------------------------------- info --------------------------------------------"
    echo "|Usage: $0 [option]                                                                     |"
    echo "|option:-r or --repourl   SVN仓库URL                                                    |"
    echo "|option:-l or --localname 本地仓库名称                                                  |"
    echo "|option:-u or --username  SVN用户名                                                     |"
    echo "|option:-p or --password  SVN密码                                                       |"
    echo "|option:-h or --help      显示帮助信息                                                  |"
    echo "|示例: $0 -r http://svn.example.com -l repo -u user -p pass                             |"
    echo "--------------------------------- 作者: gaoweiming --------------------------------------"
}

validate_params() {
    local missing=0
    [ -z "$SVN_CODE_REPO_URL" ] && { echo "错误：必须提供-r|--repourl参数"; missing=1; }
    [ -z "$SVN_CODE_REPO_LOCAL_NAME" ] && { echo "错误：必须提供-l|--localname参数"; missing=1; }
    [ -z "$SVN_CODE_REPO_USERNAME" ] && { echo "错误：必须提供-u|--username参数"; missing=1; }
    [ -z "$SVN_CODE_REPO_PASSWORD" ] && { echo "错误：必须提供-p|--password参数"; missing=1; }
    return $missing
}

build_svn_params() {
    SVN_DOWNLOAD_CODE_PARA=(
        "--username=$SVN_CODE_REPO_USERNAME"
        "--password=$SVN_CODE_REPO_PASSWORD"
        "--non-interactive"
        "--trust-server-cert"
        "$SVN_CODE_REPO_URL"
        "$SVN_CODE_REPO_LOCAL_NAME"
    )
}

cleanup_retry() {
    echo "========== 执行清理操作 =========="
    if [ -d "$SVN_CODE_REPO_LOCAL_NAME" ]; then
        cd "$SVN_CODE_REPO_LOCAL_NAME" || return 1
        svn cleanup --username="$SVN_CODE_REPO_USERNAME" --password="$SVN_CODE_REPO_PASSWORD"
        cd -
    fi
}

monitor_activity() {
    local pid=$1
    local activity_timeout=$2
    local last_activity=$(date +%s)
    
    # 启动输出监控后台进程
    {
        while read -r line; do
            last_activity=$(date +%s)
        done
    } < <(tail -f "$LOG_FILE" 2>/dev/null) &
    local monitor_pid=$!
    
    # 活动检测循环
    while kill -0 $pid 2>/dev/null; do
        sleep $MONITOR_INTERVAL
        local now=$(date +%s)
        local time_diff=$((now - last_activity))
        
        if [ $time_diff -ge $activity_timeout ]; then
            echo "========== 检测到操作停滞超过${activity_timeout}秒 =========="
            kill -TERM $pid 2>/dev/null
            kill $monitor_pid 2>/dev/null
            return 1
        fi
    done
    
    kill $monitor_pid 2>/dev/null
    wait $pid
    return $?
}

download_with_retry() {
    local retry_delay=$INITIAL_RETRY_DELAY
    local attempt=0
    local current_timeout=$INITIAL_TIMEOUT
    LOG_FILE=$(mktemp)
    
    trap 'rm -f "$LOG_FILE"; exit' EXIT
    
    while true; do
        ((attempt++))
        echo "========== 第 ${attempt} 次尝试：开始检出代码（超时时间：${current_timeout}秒） =========="
        
        cleanup_retry
        rm -f "$LOG_FILE"
        touch "$LOG_FILE"
        
        # 启动SVN进程
        svn co "${SVN_DOWNLOAD_CODE_PARA[@]}" > >(tee -a "$LOG_FILE") 2>&1 &
        local svn_pid=$!
        
        # 启动活动监控
        if monitor_activity $svn_pid $current_timeout; then
            echo "========== SVN检出成功完成 =========="
            return 0
        else
            echo "========== 操作超时或失败，准备重试 =========="
            # 动态调整超时时间
            current_timeout=$((current_timeout * 2))
            [ $current_timeout -gt $MAX_TIMEOUT ] && current_timeout=$INITIAL_TIMEOUT
            
            # 指数退避策略
            retry_delay=$((retry_delay * 2))
            [ $retry_delay -gt $MAX_RETRY_DELAY ] && retry_delay=$INITIAL_RETRY_DELAY
            echo "========== 等待 ${retry_delay} 秒后重试（下次超时时间：${current_timeout}秒） =========="
            sleep $retry_delay
        fi
    done
}

main() {
    # 参数解析
    local ARGS=$(getopt -o r:l:u:p:h --long repourl:,localname:,username:,password:,help -n "$0" -- "$@")
    eval set -- "$ARGS"

    while true; do
        case "$1" in
            -r|--repourl)    SVN_CODE_REPO_URL="$2"; shift 2 ;;
            -l|--localname)  SVN_CODE_REPO_LOCAL_NAME="$2"; shift 2 ;;
            -u|--username)   SVN_CODE_REPO_USERNAME="$2"; shift 2 ;;
            -p|--password)   SVN_CODE_REPO_PASSWORD="$2"; shift 2 ;;
            -h|--help)      usage_info; exit 0 ;;
            --)              shift; break ;;
            *)               echo "无效选项: $1"; usage_info; exit 1 ;;
        esac
    done

    validate_params || { usage_info; exit 1; }
    build_svn_params
    download_with_retry
}

main "$@"