// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings
import SwiftUI

/// A data point for a chart
protocol DataPoint {
  /// The date associated with the value on the chart
  var date: Date { get }
  /// The value to plot on a chart
  var value: CGFloat { get }
}

/// A chart that plots points data in a line over a specificed interval
struct LineChartView<DataType: DataPoint, FillStyle: View>: View {
  /// The data to plot
  private var data: [DataType]
  /// The number of columns to plot within the chart.
  ///
  /// If this is value is not the same as `data.count`, then the chart will not render edge-to-edge
  /// and can have trailing space.
  private var numberOfColumns: Int
  /// The points, plotted relatively in a [0, 1] coordinate space
  private var points: [CGPoint]
  /// The data point which is currently selected by the user
  @Binding private var selectedDataPoint: DataType?
  /// The fill applied to the line
  private var fill: FillStyle

  init(
    data: [DataType],
    numberOfColumns: Int,
    selectedDataPoint: Binding<DataType?>,
    @ViewBuilder fill: () -> FillStyle
  ) {
    assert(data.count <= numberOfColumns, "Number of data points exceed number of columns.")
    self.data = data
    self.numberOfColumns = numberOfColumns
    let (min, max) = { () -> (CGFloat, CGFloat) in
      let filledData = data.map({ $0.value })
      let min = filledData.min() ?? 0.0
      let max = filledData.max() ?? CGFloat.greatestFiniteMagnitude
      if min == max {
        // If there's only 1 value then we want to make sure we include some space above it
        // Also this will prevent dividing by 0 and causing NaN errors
        return (min, min + 1)
      }
      return (min, max)
    }()
    self.points = data.enumerated().map { index, dataPoint -> CGPoint in
      // Swift needs this pre-definition where the type is specified on lhs to compile in a
      // reasonable time (because of CGPoint generics)
      let x: CGFloat = numberOfColumns > 1 ? CGFloat(index) / CGFloat(numberOfColumns - 1) : 0
      let y: CGFloat = CGFloat(1.0) - (CGFloat(1.0) * ((dataPoint.value - min) / (max - min)))
      return .init(x: x, y: y)
    }
    self._selectedDataPoint = selectedDataPoint
    self.fill = fill()
  }

  private func point(for index: Int, in size: CGSize) -> CGPoint? {
    guard index >= 0 && index < points.count else {
      return nil
    }
    return CGPoint(
      x: points[index].x * size.width,
      y: points[index].y * size.height
    )
  }

  private struct LineChartShape: Shape {
    /// The points, plotted relatively in a [0, 1] coordinate space
    var points: [CGPoint]
    /// To close shape for the background colour
    var closed = false

    var animatableData: LineChartAnimatableData {
      get { .init(points: points.map(\.animatableData)) }
      set { points = newValue.points.map { CGPoint(x: $0.first, y: $0.second) } }
    }

    func path(in rect: CGRect) -> Path {
      Path { path in
        var previousPoint: CGPoint?
        points.forEach { relativePoint in
          let scaledPoint: CGPoint = .init(
            x: relativePoint.x * rect.width,
            y: relativePoint.y * rect.height
          )
          if path.isEmpty {
            path.move(to: scaledPoint)
          } else {
            if let previousPoint = previousPoint {
              let midpoint = previousPoint.midpoint(to: scaledPoint)
              path.addQuadCurve(to: midpoint, control: midpoint.controlPoint(to: previousPoint))
              path.addQuadCurve(to: scaledPoint, control: midpoint.controlPoint(to: scaledPoint))
            } else {
              path.addLine(to: scaledPoint)
            }
          }
          previousPoint = scaledPoint
        }
        if closed, path.currentPoint != nil {
          path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
          path.addLine(to: CGPoint(x: rect.minX, y: rect.maxY))
          path.closeSubpath()
        }
      }
    }
  }

  private struct DragContext<DataType: DataPoint> {
    var location: CGPoint
    var size: CGSize
    var dataPoint: DataType?
  }

  @State private var dragContext: DragContext<DataType>?

  private func makeDragContext(
    from offset: CGFloat,
    in size: CGSize
  ) -> DragContext<DataType>? {
    let width = size.width
    let stride = width / CGFloat(numberOfColumns - 1)
    let index = Int(round(offset / stride))
    guard index >= 0 && index < data.count,
      let calculatedPoint = point(for: index, in: size)
    else {
      return nil
    }
    return .init(
      location: calculatedPoint,
      size: size,
      dataPoint: data[index]
    )
  }

  @State private var animationScale: CGFloat = 0
  @State private var dragValueSize: CGSize = .zero

  private let dateFormatter = DateFormatter().then {
    $0.dateStyle = .short
    $0.timeStyle = .short
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      Group {
        if let dragContext = dragContext, let dataPoint = dragContext.dataPoint {
          let offsetCenter = dragContext.location.x - (dragValueSize.width / 2.0)
          let offset = max(0, min(dragContext.size.width - dragValueSize.width, offsetCenter))
          Text(dateFormatter.string(from: dataPoint.date))
            .padding(.horizontal, 8)
            .overlay(
              GeometryReader { proxy in
                Color.clear
                  .preference(key: ValueSizePreferenceKey.self, value: proxy.size)
              }
            )
            .frame(maxWidth: .infinity, alignment: .leading)
            .offset(x: offset)
        } else {
          Text(verbatim: "00:00")
            .hidden()
        }
      }
      .onPreferenceChange(ValueSizePreferenceKey.self) {
        self.dragValueSize = $0
      }
      .font(.footnote)
      ZStack {
        fill
          .clipShape(LineChartShape(points: points, closed: true))
        LineChartShape(points: points)
          .stroke(Color(.braveBlurpleTint), lineWidth: 2)
      }
      .frame(maxWidth: .infinity, alignment: .leading)
      .overlay(
        Group {
          if let dragContext = dragContext {
            ZStack {
              Rectangle()
                .fill(Color(.braveBlurpleTint))
                .frame(width: 2)
                .frame(maxHeight: .infinity)
                .padding(.vertical, -10)
                .position(x: dragContext.location.x, y: dragContext.size.height / 2)
              Circle()
                .strokeBorder(Color(.braveBlurpleTint), style: .init(lineWidth: 2))
                .frame(width: 12, height: 12)
                .background(Color(.braveBackground).clipShape(Circle()))
                .position(x: dragContext.location.x, y: dragContext.location.y)
            }
          }
        }
      )
      .overlay(
        GeometryReader { proxy in
          Color.clear
            .contentShape(Rectangle())
            .gesture(
              DragGesture(minimumDistance: 0, coordinateSpace: .local)
                .onChanged({ value in
                  dragContext = makeDragContext(
                    from: value.location.x,
                    in: proxy.size
                  )
                  selectedDataPoint = dragContext?.dataPoint
                })
                .onEnded({ _ in
                  dragContext = nil
                  selectedDataPoint = nil
                })
            )
        }
      )
    }
    .padding(.bottom, 16)
    .padding(.top, 4)
  }
}

extension CGPoint {
  /// Calculates the midpoint between this and another point
  fileprivate func midpoint(to point: CGPoint) -> CGPoint {
    .init(x: (x + point.x) / 2.0, y: (y + point.y) / 2.0)
  }

  fileprivate func controlPoint(to otherPoint: CGPoint) -> CGPoint {
    var point = midpoint(to: otherPoint)
    let diffY = abs(otherPoint.y - point.y)

    if y < otherPoint.y {
      point.y += diffY
    } else if y > otherPoint.y {
      point.y -= diffY
    }
    return point
  }
}

// MARK: - Accessibility

extension View {
  @ViewBuilder func chartAccessibility(title: String, dataPoints: [DataPoint]) -> some View {
    self
      .accessibilityChartDescriptor(LineChartDescriptor(title: title, values: dataPoints))
      .accessibilityLabel(title)
  }
}

private struct LineChartDescriptor: AXChartDescriptorRepresentable {
  var title: String
  var values: [DataPoint]

  func makeChartDescriptor() -> AXChartDescriptor {
    let (min, max) = { () -> (CGFloat, CGFloat) in
      let filledData = values.map({ $0.value })
      let min = filledData.min() ?? 0.0
      let max = filledData.max() ?? CGFloat.greatestFiniteMagnitude
      if min == max {
        // If there's only 1 value then we want to make sure we include some space above it
        // Also this will prevent dividing by 0 and causing NaN errors
        return (min, min + 1)
      }
      return (min, max)
    }()
    return AXChartDescriptor(
      title: title,
      summary: nil,
      xAxis: AXCategoricalDataAxisDescriptor(
        title: Strings.Wallet.chartAxisDateLabel,
        categoryOrder: values.map { $0.date.formatted(.dateTime) }
      ),
      yAxis: AXNumericDataAxisDescriptor(
        title: Strings.Wallet.chartAxisPriceLabel,
        range: min...max,
        gridlinePositions: [],
        valueDescriptionProvider: { value in
          "\(value)"
        }
      ),
      additionalAxes: [],
      series: [
        AXDataSeriesDescriptor(
          name: "",
          isContinuous: true,
          dataPoints: values.map {
            AXDataPoint(
              x: $0.date.formatted(.dateTime),
              y: $0.value,
              additionalValues: [],
              label: nil
            )
          }
        )
      ]
    )
  }
}

// MARK: - Animation

/// Animatable data for a line chart
private struct LineChartAnimatableData: VectorArithmetic {
  /// The points to plot in the chart
  var points: [CGPoint.AnimatableData]

  /// Applies a function between two animatable data sets
  static func animatableData(
    lhs: Self,
    rhs: Self,
    applying function: (CGPoint.AnimatableData, CGPoint.AnimatableData) -> CGPoint.AnimatableData
  ) -> Self {
    var points: [CGPoint.AnimatableData] = []
    let minCount = min(lhs.points.count, rhs.points.count)
    let maxCount = max(lhs.points.count, rhs.points.count)
    for index in 0..<maxCount {
      if index < minCount {
        // Merge points
        points.append(function(lhs.points[index], rhs.points[index]))
      } else if rhs.points.count > lhs.points.count, let lastLeftPoint = lhs.points.last {
        // Right side has more points, collapse to last left point
        points.append(function(lastLeftPoint, rhs.points[index]))
      } else if let lastPoint = points.last, index < lhs.points.count {
        // Left side has more points, collapse to last known point
        points.append(function(lastPoint, lhs.points[index]))
      }
      // Thanks to https://stackoverflow.com/q/64157672 for collapsing points implementation detail
    }

    return .init(points: points)
  }

  static func + (lhs: Self, rhs: Self) -> Self {
    animatableData(lhs: lhs, rhs: rhs, applying: +)
  }

  static func - (lhs: Self, rhs: Self) -> Self {
    animatableData(lhs: lhs, rhs: rhs, applying: -)
  }

  mutating func scale(by rhs: Double) {
    points.indices.forEach { index in
      self.points[index].scale(by: rhs)
    }
  }

  var magnitudeSquared: Double {
    return points.reduce(0, { $0 + $1.magnitudeSquared })
  }

  static var zero: LineChartAnimatableData {
    return .init(points: [])
  }
}

private struct ValueSizePreferenceKey: PreferenceKey {
  static var defaultValue: CGSize = .zero
  static func reduce(value: inout CGSize, nextValue: () -> CGSize) {
    value = nextValue()
  }
}
