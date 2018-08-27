#include "controlsbox.h"

ControlsBox::ControlsBox(FrameWorker *fw, QTabWidget *tw,
                         QString ipAddress, quint16 port, QWidget *parent) :
    QWidget(parent), collecting_mask(false)
{
    frame_handler = fw;
    connect(frame_handler, &FrameWorker::updateFPS,
            this, &ControlsBox::updateFPS);
    tab_handler = tw;
    connect(tab_handler, SIGNAL(currentChanged(int)),
            this, SLOT(tabChanged(int)));
    viewWidget = getCurrentTab();

    fpsLabel = new QLabel("Warning: No Frames Received");
    fpsLabel->setFixedWidth(200);

    QLabel *ipLabel = new QLabel(QString("IP Address: %1").arg(ipAddress), this);
    QLabel *portLabel = new QLabel(QString("Port Label: %1").arg(port), this);

    rangeSlider = new ctkRangeSlider(this);
    rangeSlider->setOrientation(Qt::Horizontal);
    connect(rangeSlider, &ctkRangeSlider::minimumPositionChanged, viewWidget, &LVTabApplication::setFloorPos);
    connect(rangeSlider, &ctkRangeSlider::maximumPositionChanged, viewWidget, &LVTabApplication::setCeilingPos);

    min_box = new QSpinBox(this);
    max_box = new QSpinBox(this);

    precisionBox = new QCheckBox("Precision Slider", this);
    connect(precisionBox, SIGNAL(toggled(bool)), this, SLOT(setPrecision(bool)));

    maskButton = new QPushButton("&Collect Mask Frames", this);
    connect(maskButton, &QPushButton::released, this, &ControlsBox::collectDSFMask);
    connect(frame_handler->DSFilter, &DarkSubFilter::mask_frames_collected, this, [this](){
        this->collectDSFMask();
    });

    QGridLayout *cboxLayout = new QGridLayout(this);
    cboxLayout->addWidget(fpsLabel, 0, 0, 1, 1);
    cboxLayout->addWidget(ipLabel, 1, 0, 1, 1);
    cboxLayout->addWidget(portLabel, 2, 0, 1, 1);
    cboxLayout->addWidget(new QLabel("Range:"), 0, 1, 1, 1);
    cboxLayout->addWidget(min_box, 0, 2, 1, 1);
    cboxLayout->addWidget(rangeSlider, 0, 3, 1, 5);
    cboxLayout->addWidget(max_box, 0, 8, 1, 1);
    cboxLayout->addWidget(precisionBox, 0, 9, 1, 2);
    cboxLayout->addWidget(maskButton, 1, 9, 1, 1);
    this->setLayout(cboxLayout);
    this->setMaximumHeight(100);
    tabChanged(0);

    connect(rangeSlider, &ctkRangeSlider::minimumPositionChanged, this, &ControlsBox::setMinSpin);
    connect(rangeSlider, &ctkRangeSlider::maximumPositionChanged, this, &ControlsBox::setMaxSpin);

    connect(min_box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ControlsBox::setRangeSliderMin);
    connect(max_box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ControlsBox::setRangeSliderMax);

    connect(min_box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int new_min){
        if(new_min <= viewWidget->getCeiling())
            viewWidget->setFloor(new_min);
    });
    connect(max_box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int new_max) {
        if(new_max >= viewWidget->getFloor())
            viewWidget->setCeiling(new_max);
    });
}

void ControlsBox::setMinSpin(int new_min) {
    min_box->blockSignals(true);
    min_box->setValue(static_cast<int>(new_min * viewWidget->getDataMax() / 100.0));
    min_box->blockSignals(false);
}

void ControlsBox::setMaxSpin(int new_max) {
    max_box->blockSignals(true);
    max_box->setValue(static_cast<int>(new_max * viewWidget->getDataMax() / 100.0));
    max_box->blockSignals(false);
}

void ControlsBox::setRangeSliderMin(int new_min) {
    if(new_min <= max_box->value()) {
        rangeSlider->blockSignals(true);
        rangeSlider->setMinimumPosition(static_cast<int>(new_min * 100.0 / viewWidget->getDataMax()));
        rangeSlider->blockSignals(false);
    }
}

void ControlsBox::setRangeSliderMax(int new_max) {
    if(new_max >= min_box->value()) {
        rangeSlider->blockSignals(true);
        rangeSlider->setMaximumPosition(static_cast<int>(new_max * 100.0 / viewWidget->getDataMax()));
        rangeSlider->blockSignals(false);
    }
}

ControlsBox::~ControlsBox() {}

void ControlsBox::tabChanged(int index)
{
    Q_UNUSED( index );

    // disconnect the signals associated with the pointer to the old tab
    disconnect(rangeSlider, &ctkRangeSlider::minimumPositionChanged,
               viewWidget, &LVTabApplication::setFloorPos);
    disconnect(rangeSlider, &ctkRangeSlider::maximumPositionChanged,
               viewWidget, &LVTabApplication::setCeilingPos);

    // associate the GUI items with the pointer to the new tab
    viewWidget = getCurrentTab();
    if(!viewWidget) {
        return;
    }

    connect(rangeSlider, &ctkRangeSlider::minimumPositionChanged,
            viewWidget, &LVTabApplication::setFloorPos);
    connect(rangeSlider, &ctkRangeSlider::maximumPositionChanged,
            viewWidget, &LVTabApplication::setCeilingPos);

    precisionBox->setChecked(viewWidget->isPrecisionMode());

    // update the range slider positions
    rangeSlider->setPositions(static_cast<int>(viewWidget->getFloor() / viewWidget->getDataMax() * 100.0),
                              static_cast<int>(viewWidget->getCeiling() / viewWidget->getDataMax() * 100.0));

    min_box->setMinimum(static_cast<int>(viewWidget->getDataMin()));
    min_box->setMaximum(static_cast<int>(viewWidget->getDataMax()));
    min_box->setValue(static_cast<int>(viewWidget->getFloor()));

    max_box->setMinimum(static_cast<int>(viewWidget->getDataMin()));
    max_box->setMaximum(static_cast<int>(viewWidget->getDataMax()));
    max_box->setValue(static_cast<int>(viewWidget->getCeiling()));
}

void ControlsBox::setPrecision(bool isPrecise)
{
    viewWidget->setPrecision(isPrecise);

    if (isPrecise) {
        // Set the minimum position out of 100, so the slider can go as low as
        // -dataMax
        rangeSlider->setMinimum(-100);
    } else {
        rangeSlider->setMinimum(0);
    }

    // update the range slider positions
    rangeSlider->setPositions(static_cast<int>(viewWidget->getFloor() / viewWidget->getDataMax() * 100.0),
                              static_cast<int>(viewWidget->getCeiling() / viewWidget->getDataMax() * 100.0));

    min_box->setMinimum(static_cast<int>(viewWidget->getDataMin()));
    min_box->setMaximum(static_cast<int>(viewWidget->getDataMax()));
    min_box->setValue(static_cast<int>(viewWidget->getFloor()));

    max_box->setMinimum(static_cast<int>(viewWidget->getDataMin()));
    max_box->setMaximum(static_cast<int>(viewWidget->getDataMax()));
    max_box->setValue(static_cast<int>(viewWidget->getCeiling()));
}

void ControlsBox::collectDSFMask()
{
    collecting_mask = !collecting_mask;
    if (collecting_mask) {
        maskButton->setText("&Stop Collecting Mask");
        frame_handler->collectMask();
    } else {
        maskButton->setText("&Collect Mask Frames");
        frame_handler->stopCollectingMask();
    }
}

void ControlsBox::updateFPS(float frameRate)
{
    if (static_cast<int>(frameRate) == -1) {
        fpsLabel->setText(QString("Warning: No Frames Received"));
    } else {
        fpsLabel->setText(QString("FPS @ backend: %1")
                          .arg(QString::number(frameRate, 'f', 1)));
    }
}

LVTabApplication* ControlsBox::getCurrentTab()
{
    return qobject_cast<LVTabApplication*>(tab_handler->widget(
                                               tab_handler->currentIndex()));
}
