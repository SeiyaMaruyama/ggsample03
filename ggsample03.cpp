//
// ゲームグラフィックス特論宿題アプリケーション
//
#include "GgApp.h"
#include <cmath>

// プロジェクト名
#ifndef PROJECT_NAME
#  define PROJECT_NAME "ggsample03"
#endif

// オブジェクト関連の処理
#include "object.h"

// シェーダー関連の処理
#include "shader.h"

//
// 4 行 4 列の行列の積を求める
//
//   m ← m1 × m2
//
static void multiply(GLfloat* m, const GLfloat* m1, const GLfloat* m2)
{
  for (int i = 0; i < 16; ++i)
  {
    int j = i & 3, k = i & ~3;

    // 配列変数に行列が転置された状態で格納されていることを考慮している
    m[i] = m1[0 + j] * m2[k + 0] + m1[4 + j] * m2[k + 1] + m1[8 + j] * m2[k + 2] + m1[12 + j] * m2[k + 3];
  }
}

//
// 単位行列を設定する
//
//   m: 単位行列を格納する配列
//
static void loadIdentity(GLfloat* m)
{
  m[ 0] = m[ 5] = m[10] = m[15] = 1.0f;
  m[ 1] = m[ 2] = m[ 3] = m[ 4] =
  m[ 6] = m[ 7] = m[ 8] = m[ 9] =
  m[11] = m[12] = m[13] = m[14] = 0.0f;
}

//
// 直交投影変換行列を求める
//
//   m: 直交投影変換行列を格納する配列
//   left, right: ビューボリュームの左右端
//   bottom, top: ビューボリュームの上下端
//   zNear, zFar: 前方面および後方面までの距離
//
static void ortho(GLfloat* m, float left, float right, float bottom, float top, float zNear, float zFar)
{
  m[ 0] = 2.0f / (right - left);
  m[ 5] = 2.0f / (top - bottom);
  m[10] = -2.0f / (zFar - zNear);
  m[12] = -(right + left) / (right - left);
  m[13] = -(top + bottom) / (top - bottom);
  m[14] = -(zFar + zNear) / (zFar - zNear);
  m[15] = 1.0f;
  m[ 1] = m[ 2] = m[ 3] = m[ 4] = m[ 6] = m[ 7] = m[ 8] = m[ 9] = m[11] = 0.0f;
}

//
// 透視投影変換行列を求める
//
//   m: 透視投影変換行列を格納する配列
//   left, right: 前方面の左右端
//   bottom, top: 前方面の上下端
//   zNear, zFar: 前方面および後方面までの距離
//
static void frustum(GLfloat* m, float left, float right, float bottom, float top, float zNear, float zFar)
{
  m[ 0] = 2.0f * zNear / (right - left);
  m[ 5] = 2.0f * zNear / (top - bottom);
  m[ 8] = (right + left) / (right - left);
  m[ 9] = (top + bottom) / (top - bottom);
  m[10] = -(zFar + zNear) / (zFar - zNear);
  m[11] = -1.0f;
  m[14] = -2.0f * zFar * zNear / (zFar - zNear);
  m[ 1] = m[ 2] = m[ 3] = m[ 4] = m[ 6] = m[ 7] = m[12] = m[13] = m[15] = 0.0f;
}

//
// 画角を指定して透視投影変換行列を求める
//
//   m: 透視投影変換行列を格納する配列
//   fovy: 画角（ラジアン）
//   aspect: ウィンドウの縦横比
//   zNear, zFar: 前方面および後方面までの距離
//
static void perspective(GLfloat* m, float fovy, float aspect, float zNear, float zFar)
{
  const float theta = fovy * 0.5f * M_PI / 180.0f; // 視野角をラジアンに変換する
  const float a = aspect;
  const float dz = zFar - zNear;
  const float cot = 1.0f / tan(theta);
  m[0] = cot / a;
  m[1] = 0.0f;
  m[2] = 0.0f;
  m[3] = 0.0f;
  m[4] = 0.0f;
  m[5] = cot;
  m[6] = 0.0f;
  m[7] = 0.0f;
  m[8] = 0.0f;
  m[9] = 0.0f;
  m[10] = -(zFar + zNear) / dz;
  m[11] = -(2.0f * zFar * zNear) / dz;
  m[12] = 0.0f;
  m[13] = 0.0f;
  m[14] = -1.0f;
  m[15] = 0.0f;
}

//
// ビュー変換行列を求める
//
//   m: ビュー変換行列を格納する配列
//   ex, ey, ez: 視点の位置
//   tx, ty, tz: 目標点の位置
//   ux, uy, uz: 上方向のベクトル
//
static void lookat(GLfloat* m, float ex, float ey, float ez, float tx, float ty, float tz, float ux, float uy, float uz)
{
  // 視点の位置を計算
  float fx = tx - ex;
  float fy = ty - ey;
  float fz = tz - ez;

  // forwardベクトルを正規化
  float norm = sqrt(fx * fx + fy * fy + fz * fz);
  fx /= norm;
  fy /= norm;
  fz /= norm;

  // upベクトルを正規化
  norm = sqrt(ux * ux + uy * uy + uz * uz);
  ux /= norm;
  uy /= norm;
  uz /= norm;

  // sベクトルを計算
  float sx = fy * uz - fz * uy;
  float sy = fz * ux - fx * uz;
  float sz = fx * uy - fy * ux;

  // sベクトルを正規化
  norm = sqrt(sx * sx + sy * sy + sz * sz);
  sx /= norm;
  sy /= norm;
  sz /= norm;

  // uベクトルを計算
  ux = sy * fz - sz * fy;
  uy = sz * fx - sx * fz;
  uz = sx * fy - sy * fx;

  // カメラの姿勢行列を作成
  m[0] = sx;
  m[1] = ux;
  m[2] = -fx;
  m[3] = 0.0f;

  m[4] = sy;
  m[5] = uy;
  m[6] = -fy;
  m[7] = 0.0f;

  m[8] = sz;
  m[9] = uz;
  m[10] = -fz;
  m[11] = 0.0f;

  m[12] = -(sx * ex + sy * ey + sz * ez);
  m[13] = -(ux * ex + uy * ey + uz * ez);
  m[14] = fx * ex + fy * ey + fz * ez;
  m[15] = 1.0f;
}

//透視投影変換行列とビュー変換行列の積を求める用
//static void multMatrix(GLfloat* result, const GLfloat* a, const GLfloat* b) {
//  for(int i = 0; i < 4; ++i) {
//    const int ai0 = i, ai1 = i + 4, ai2 = i + 8, ai3 = i + 12;
//    result[ai0] = a[ai0] * b[0] + a[ai1] * b[4] + a[ai2] * b[8] + a[ai3] * b[12];
//    result[ai1] = a[ai0] * b[1] + a[ai1] * b[5] + a[ai2] * b[9] + a[ai3] * b[13];
//    result[ai2] = a[ai0] * b[2] + a[ai1] * b[6] + a[ai2] * b[10] + a[ai3] * b[14];
//    result[ai3] = a[ai0] * b[3] + a[ai1] * b[7] + a[ai2] * b[11] + a[ai3] * b[15];
//  }
//}

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // ウィンドウを作成する (この行は変更しないでください)
  Window window{ argc > 1 ? argv[1] : PROJECT_NAME };

  // 背景色を指定する
  glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

  // プログラムオブジェクトの作成
  const auto program{ loadProgram(PROJECT_NAME ".vert", "pv", PROJECT_NAME ".frag", "fc") };

  // uniform 変数のインデックスの検索（見つからなければ -1）
  const auto mcLoc{ glGetUniformLocation(program, "mc") };

  // ビュー変換行列を mv に求める
  GLfloat mv[16];
  lookat(mv, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // 頂点属性
  static const GLfloat position[][3]
  {
    { -0.9f, -0.9f, -0.9f },  // (0)
    {  0.9f, -0.9f, -0.9f },  // (1)
    {  0.9f, -0.9f,  0.9f },  // (2)
    { -0.9f, -0.9f,  0.9f },  // (3)
    { -0.9f,  0.9f, -0.9f },  // (4)
    {  0.9f,  0.9f, -0.9f },  // (5)
    {  0.9f,  0.9f,  0.9f },  // (6)
    { -0.9f,  0.9f,  0.9f },  // (7)
  };

  // 頂点数
  constexpr auto vertices{ static_cast<GLuint>(std::size(position)) };

  // 頂点インデックス
  static const GLuint index[]
  {
    0, 1,
    1, 2,
    2, 3,
    3, 0,
    0, 4,
    1, 5,
    2, 6,
    3, 7,
    4, 5,
    5, 6,
    6, 7,
    7, 4,
  };

  // 稜線数
  constexpr auto lines{ static_cast<GLuint>(std::size(index)) };

  // 頂点配列オブジェクトの作成
  const auto vao{ createObject(vertices, position, lines, index) };

  // ウィンドウが開いている間繰り返す
  while (window)
  {
    // ウィンドウを消去する
    glClear(GL_COLOR_BUFFER_BIT);

    // シェーダプログラムの使用開始
    glUseProgram(program);

    // 投影変換行列 mp を求める (window.getAspect() はウィンドウの縦横比)
    GLfloat mp[16];
    perspective(mp, 0.5f, window.getAspect(), 1.0f, 15.0f);

    // 投影変換行列 mp とビュー変換行列 mv の積を変換行列 mc に求める
    GLfloat mc[16];
    multiply(mc, mp, mv);

    // uniform 変数 mc に変換行列 mc を設定する
    glUniformMatrix4fv(mcLoc, 1, GL_FALSE, mc);

    // 描画に使う頂点配列オブジェクトの指定
    glBindVertexArray(vao);

    // 図形の描画
    glDrawElements(GL_LINES, lines, GL_UNSIGNED_INT, 0);

    // 頂点配列オブジェクトの指定解除
    glBindVertexArray(0);

    // シェーダプログラムの使用終了
    glUseProgram(0);

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
