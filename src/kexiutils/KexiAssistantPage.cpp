/* This file is part of the KDE project
   Copyright (C) 2011-2018 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KexiAssistantPage.h"

#include "utils.h"
#include "KexiTitleLabel.h"
#include "KexiLinkWidget.h"
#include "KexiLinkButton.h"
#include "KexiCloseButton.h"

#include <KexiIcon.h>

#include <KAcceleratorManager>
#include <KStandardGuiItem>
#include <KLocalizedString>

#include <QGridLayout>
#include <QLineEdit>
#include <QPointer>

class Q_DECL_HIDDEN KexiAssistantPage::Private {
public:
    explicit Private(KexiAssistantPage* q_) : q(q_), backButton(0), nextButton(0)
    {
    }
    void setButtonVisible(KexiLinkWidget** button, bool back, bool set,
                          int x, int y);
    QColor linkColor() const;

    QLineEdit *recentFocusLineEdit() { return qobject_cast<QLineEdit*>(recentFocusWidget); }

    KexiAssistantPage * const q;
    QGridLayout* mainLyr;
    KexiTitleLabel* titleLabel;
    QLabel* descriptionLabel;
    KexiLinkWidget* backButton;
    KexiLinkWidget* nextButton;
    KexiCloseButton* cancelButton;
    QPointer<QWidget> recentFocusWidget;
    int recentFocusLineEditSelectionStart = -1;
    int recentFocusLineEditSelectionLength = -1;
    int recentFocusLineEditCursorPosition = -1;
};

void KexiAssistantPage::Private::setButtonVisible(KexiLinkWidget** button,
                                                  bool back, /* or next */
                                                  bool set, int x, int y)
{
    if (set) {
        if (*button) {
            (*button)->show();
        }
        else {
            if (back) {
                *button = new KexiLinkWidget(
                    QLatin1String("KexiAssistantPage:back"),
                    KStandardGuiItem::back().plainText(), q);
                (*button)->setFormat(
                    xi18nc("Back button arrow: back button in assistant (wizard)", "‹ %L"));
            }
            else {
                *button = new KexiLinkWidget(
                    QLatin1String("KexiAssistantPage:next"),
                    xi18nc("Button text: Next page in assistant (wizard)", "Next"), q);
                (*button)->setFormat(
                    xi18nc("Next button arrow: next button in assistant (wizard)", "%L ›"));
            }
            int space = (*button)->fontMetrics().height() / 2;
            Qt::Alignment align;
            if (back) {
                (*button)->setContentsMargins(0, 0, space, 0);
                align = Qt::AlignTop | Qt::AlignLeft;
            }
            else {
                (*button)->setContentsMargins(space, 0, 0, 0);
                align = Qt::AlignTop | Qt::AlignRight;
            }
            KAcceleratorManager::setNoAccel(*button);
            mainLyr->addWidget(*button, x, y, align);
            connect(*button, SIGNAL(linkActivated(QString)),
                    q, SLOT(slotLinkActivated(QString)));
        }
    }
    else {
        if (*button)
            (*button)->hide();
    }
}

// ----

KexiAssistantPage::KexiAssistantPage(const QString& title, const QString& description, QWidget* parent)
 : QWidget(parent)
 , d(new Private(this))
{
/*0         [titleLabel]       [cancel]
  1  [back] [descriptionLabel]   [next]
  2         [contents]                 */
    d->mainLyr = new QGridLayout(this);
    d->mainLyr->setContentsMargins(0, 0, 0, 0);
    d->mainLyr->setColumnStretch(1, 1);
    d->mainLyr->setRowStretch(2, 1);
    d->titleLabel = new KexiTitleLabel(title);
    d->mainLyr->addWidget(d->titleLabel, 0, 1, Qt::AlignTop);
    d->descriptionLabel = new QLabel(description);
    int space = d->descriptionLabel->fontMetrics().height();
    d->descriptionLabel->setContentsMargins(2, 0, 0, space);
    d->descriptionLabel->setWordWrap(true);
    d->mainLyr->addWidget(d->descriptionLabel, 1, 1, Qt::AlignTop);

    d->cancelButton = new KexiCloseButton;
    connect(d->cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()));
    d->mainLyr->addWidget(d->cancelButton, 0, 2, Qt::AlignTop|Qt::AlignRight);
}

KexiAssistantPage::~KexiAssistantPage()
{
    delete d;
}

void KexiAssistantPage::setDescription(const QString& text)
{
    d->descriptionLabel->setText(text);
}

void KexiAssistantPage::setBackButtonVisible(bool set)
{
    d->setButtonVisible(&d->backButton, true/*back*/, set, 1, 0);
}

void KexiAssistantPage::setNextButtonVisible(bool set)
{
    d->setButtonVisible(&d->nextButton, false/*next*/, set, 1, 2);
}

void KexiAssistantPage::setContents(QWidget* widget)
{
    widget->setContentsMargins(0, 0, 0, 0);
    d->mainLyr->addWidget(widget, 2, 1, 2, 2);
}

void KexiAssistantPage::setContents(QLayout* layout)
{
    layout->setContentsMargins(0, 0, 0, 0);
    d->mainLyr->addLayout(layout, 2, 1);
}

void KexiAssistantPage::slotLinkActivated(const QString& link)
{
    if (d->backButton && link == d->backButton->link()) {
        back();
    }
    else if (d->nextButton && link == d->nextButton->link()) {
        next();
    }
}

void KexiAssistantPage::slotCancel()
{
    emit cancelledRequested(this);
    if (parentWidget()) {
        parentWidget()->deleteLater();
    }
}

KexiLinkWidget* KexiAssistantPage::backButton()
{
    if (!d->backButton) {
        setBackButtonVisible(true);
        d->backButton->hide();
    }
    return d->backButton;
}

KexiLinkWidget* KexiAssistantPage::nextButton()
{
    if (!d->nextButton) {
        setNextButtonVisible(true);
        d->nextButton->hide();
    }
    return d->nextButton;
}

void KexiAssistantPage::tryBack()
{
    emit tryBackRequested(this);
}

void KexiAssistantPage::back()
{
    emit backRequested(this);
}

void KexiAssistantPage::next()
{
    emit nextRequested(this);
}

QWidget* KexiAssistantPage::recentFocusWidget() const
{
    return d->recentFocusWidget;
}

void KexiAssistantPage::setRecentFocusWidget(QWidget* widget)
{
    d->recentFocusWidget = widget;
    QLineEdit *edit = d->recentFocusLineEdit();
    d->recentFocusLineEditSelectionStart = edit ? edit->selectionStart() : -1;
    d->recentFocusLineEditSelectionLength = (edit && edit->hasSelectedText()) ? edit->selectedText().length() : -1;
    d->recentFocusLineEditCursorPosition = edit ? edit->cursorPosition() : -1;
}

void KexiAssistantPage::focusRecentFocusWidget()
{
    if (!d->recentFocusWidget) {
        return;
    }
    d->recentFocusWidget->setFocus();
    QLineEdit *edit = d->recentFocusLineEdit();
    if (edit && d->recentFocusLineEditSelectionStart >= 0
        && d->recentFocusLineEditSelectionLength >= 0)
    {
        edit->setCursorPosition(d->recentFocusLineEditCursorPosition);
        edit->setSelection(d->recentFocusLineEditSelectionStart, d->recentFocusLineEditSelectionLength);
    }
}

QString KexiAssistantPage::title() const
{
    return d->titleLabel->text();
}

QString KexiAssistantPage::description() const
{
    return d->descriptionLabel->text();
}

