#include "zhuziWin32.h"

#pragma comment(lib,"zhuziWin32.lib")

using namespace zhuzi;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    zhuziInstance inst(hInstance);
    zhuziWindow mainWnd;
    mainWnd.create(L"二叉树展示示例", 100, 100, 600, 400);
    mainWnd.setBgColor(RGB(240, 240, 240));

    zhuziStatusBar sta(&mainWnd);
    sta.create();
    sta.setParts({ -1 });
    sta.setPartText(0, L"小提示:鼠标右键有惊喜!");

    // 2. 创建 TreeView 控件
    zhuziTreeView treeView(&mainWnd);
    treeView.create(10U,10U,10U,30U);
    treeView.setLines(1);
    treeView.setButtons(1);
    treeView.setLinesAtRoot(1);

    zhuziBinaryTree<zhuziString> tree;
    auto root = tree.createNode(L"中国");
    tree.setRoot(root);
    auto zhejiang = tree.createNode(L"浙江");
    tree.setLeft(root, zhejiang);
    auto hangzhou = tree.createNode(L"杭州");
    tree.setLeft(zhejiang, hangzhou);
    auto shaoxing = tree.createNode(L"绍兴");
    tree.setRight(zhejiang, shaoxing);
    auto xihu = tree.createNode(L"西湖区");
    tree.setLeft(hangzhou, xihu);
    auto shangc = tree.createNode(L"上城区");
    tree.setRight(hangzhou, shangc);

    treeView.addFromBTree(tree); // 现在 TreeView 将显示正确的层次结构
    treeView.setAutoRedraw(0);
    auto zhongguo = treeView.getRootItem();
    auto zhej = treeView.getChildItem(zhongguo);
    treeView.insertItem(zhej, L"金华");
    auto hangz = treeView.getChildItem(zhej);
    treeView.insertItem(hangz, L"萧山区");
    auto hunan = treeView.insertItem(zhongguo,L"湖南");
    auto changsha = treeView.insertItem(hunan, L"长沙");
    treeView.insertItem(changsha, L"岳麓区");
    treeView.insertItem(changsha, L"望城区");
    auto shaoyang = treeView.insertItem(hunan, L"邵阳");
    auto shaodong = treeView.insertItem(shaoyang, L"邵东");
    treeView.insertItem(shaodong, L"两市镇");
    treeView.insertItem(shaodong, L"砂石镇");
    treeView.insertItem(shaoyang, L"双清区");
    treeView.insertItem(shaoyang, L"大祥区");
    treeView.setAutoRedraw(1);
    treeView.setOnRClick([&](zhuziTreeItem ti) {
        MessageBoxW(mainWnd.getHandle(), (L"你选中了" + treeView.getItemText(ti)).c_str(), L"警告", MB_ICONWARNING);
        });
    return inst.run(mainWnd);
}