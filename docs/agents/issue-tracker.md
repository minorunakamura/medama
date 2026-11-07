# Issue tracker

## Tracker

GitHub Issues

## Repository

- Host: github.com
- Repository: minorunakamura/medama
- URL: https://github.com/minorunakamura/medama
- Visibility: PUBLIC。Issue の作成は一般公開を伴う。

## 作成・取得

- 公開 gh CLI を使い、常に minorunakamura/medama を明示する。
- 作成: gh issue create --repo minorunakamura/medama --title <title> --body <body> --label ready-for-agent
- 取得: gh issue view <number-or-url> --repo minorunakamura/medama --json number,url,title,body,comments,labels,state,blockedBy,blocking
- 使用ラベルは ready-for-agent。未作成なら別途承認後に作成する。
- 各 Issue に、repository を特定した正式な Approved Plan のプロジェクト内パスを記す。未公開ファイルの blob URL を捏造しない。

## 依存関係

blocker を先に作成し、本文の Blocked by に実 Issue のリンクを記す。
利用可能な gh CLI の --blocked-by で native blocking relationship も設定し、取得して確認する。
機能自体が非対応と確認できた場合のみ本文リンクで表す。
認証・権限・通信の失敗では停止する。元 Issue の本文・状態は変更しない。
