#include "aboutdialog.h"
#include "cutils.h"

#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("AboutDialog"));
    setWindowTitle(tr("About EANScannerEmu"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowIcon(QIcon(QStringLiteral(":/icon.png")));
    setMinimumWidth(460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 20);
    layout->setSpacing(16);

    auto *header = new QHBoxLayout;
    header->setSpacing(18);
    auto *icon = new QLabel(this);
    icon->setPixmap(windowIcon().pixmap(QSize(72, 72), devicePixelRatioF()));
    icon->setFixedSize(72, 72);
    header->addWidget(icon);

    auto *identity = new QVBoxLayout;
    identity->setSpacing(8);
    auto *eyebrow = new QLabel(tr("BARCODE SCANNER EMULATOR"), this);
    eyebrow->setObjectName(QStringLiteral("AboutEyebrow"));
    identity->addWidget(eyebrow);
    auto *title = new QLabel(QStringLiteral("EANScannerEmu"), this);
    title->setObjectName(QStringLiteral("AboutTitle"));
    identity->addWidget(title);
    auto *version = new QLabel(tr("Version %1").arg(CUtils::GetVersion()), this);
    version->setObjectName(QStringLiteral("AboutVersion"));
    identity->addWidget(version, 0, Qt::AlignLeft);
    header->addLayout(identity, 1);
    layout->addLayout(header);

    auto *description = new QLabel(tr("Emulate a barcode scanner for testing and automation."), this);
    description->setObjectName(QStringLiteral("AboutDescription"));
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *detailsCard = new QFrame(this);
    detailsCard->setObjectName(QStringLiteral("AboutDetails"));
    auto *details = new QGridLayout(detailsCard);
    details->setContentsMargins(16, 14, 16, 14);
    details->setHorizontalSpacing(24);
    details->setVerticalSpacing(10);
    const auto addDetail = [this, details](int row, const QString &name, const QString &value) {
        auto *label = new QLabel(name, this);
        label->setProperty("aboutDetailLabel", true);
        details->addWidget(label, row, 0);
        auto *text = new QLabel(value, this);
        text->setWordWrap(true);
        details->addWidget(text, row, 1);
    };
    addDetail(0, tr("Created by"), QStringLiteral("Mikhail Bersenev"));
    addDetail(1, tr("License"), tr("GNU GPL v3"));
    details->setColumnStretch(1, 1);
    layout->addWidget(detailsCard);

    auto *links = new QHBoxLayout;
    links->setSpacing(8);
    const auto addLink = [this, links](const QString &text, const QString &url) {
        auto *button = new QPushButton(text, this);
        button->setProperty("aboutLink", true);
        button->setAutoDefault(false);
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(url);
        connect(button, &QPushButton::clicked, this, [url] {
            QDesktopServices::openUrl(QUrl(url));
        });
        links->addWidget(button);
    };
    addLink(tr("GitHub"), QStringLiteral("https://github.com/MikhailBersenev/EANScannerEmu"));
    addLink(tr("Website"), QStringLiteral("http://mbersenev.ph/"));
    links->addStretch();
    layout->addLayout(links);

    auto *divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("AboutDivider"));
    divider->setFixedHeight(1);
    layout->addWidget(divider);

    auto *footer = new QHBoxLayout;
    auto *copyright = new QLabel(tr("© 2026 Mikhail Bersenev"), this);
    copyright->setObjectName(QStringLiteral("AboutCopyright"));
    footer->addWidget(copyright);
    footer->addStretch();
    auto *close = new QPushButton(tr("Close"), this);
    close->setObjectName(QStringLiteral("AboutClose"));
    close->setDefault(true);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(close);
    layout->addLayout(footer);
    close->setFocus();
}
