#include "infoframe.h"
#include <QHBoxLayout>

InfoFrame::InfoFrame(QWidget *parent)
    : QFrame(parent)
{
  auto *layout = new QHBoxLayout(this);

  m_icon = new QPushButton(this);
  m_icon->setFlat(true);
  m_icon->setIconSize(QSize(32, 32));
  m_icon->setMaximumSize(32, 32);
  m_icon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  layout->addWidget(m_icon);

  auto *spacer = new QSpacerItem(5, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
  layout->addSpacerItem(spacer);

  m_infoLabel = new QLabel(this);
  m_infoLabel->setWordWrap(true);
  m_infoLabel->setTextFormat(Qt::MarkdownText);
  m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
  layout->addWidget(m_infoLabel);
}

void InfoFrame::setInfo(const QIcon &icon, const QString &text) {
  m_icon->setIcon(icon);
  m_infoLabel->setText(text);

#ifdef Q_OS_MACOS
  this->setFrameShape(QFrame::Box);
#else
  this->setFrameShape(QFrame::StyledPanel);
#endif
}

void InfoFrame::setText(const QString &text) {
  m_infoLabel->setText(text);
}
