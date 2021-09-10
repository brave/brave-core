/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI

/// A data point for a chart
protocol DataPoint {
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
      return (filledData.min() ?? 0.0,
              filledData.max() ?? CGFloat.greatestFiniteMagnitude)
    }()
    self.points = data.enumerated().map { index, dataPoint -> CGPoint in
      // Swift needs this pre-definition where the type is specified on lhs to compile in a
      // reasonable time (because of CGPoint generics)
      let x: CGFloat = CGFloat(index) / CGFloat(numberOfColumns  - 1)
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
      }
      .strokedPath(StrokeStyle(lineWidth: 3, lineCap: .round, lineJoin: .round))
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
          let calculatedPoint = point(for: index, in: size) else {
      return nil
    }
    return .init(
      location: calculatedPoint,
      size: size,
      dataPoint: data[index]
    )
  }
  
  @State private var animationScale: CGFloat = 0
  
  var finalDotView: some View {
    let size: CGFloat = 14.0
    return GeometryReader { proxy in
      if numberOfColumns != points.count,
         let scaledPoint = point(for: points.endIndex - 1, in: proxy.size) {
        Circle()
          .frame(width: size, height: size)
          .background(
            Circle()
              .opacity((1.0 - Double(animationScale)) * 0.6)
              // Can't scale effect to 0, causes bugs
              .scaleEffect(max(animationScale, 0.1) * 2.5)
              .animation(Animation.linear(duration: 2).repeatForever(autoreverses: false), value: animationScale)
          )
          .frame(maxWidth: .infinity, alignment: .leading)
          .offset(x: scaledPoint.x - (size / 2.0), y: scaledPoint.y - (size / 2.0))
          .transition(
            .asymmetric(
              insertion: AnyTransition.opacity.animation(Animation.linear(duration: 0.1).delay(0.2)),
              removal: AnyTransition.opacity.animation(.linear(duration: 0.1))
            )
          )
          .onAppear {
            animationScale = 1
          }
          .onDisappear {
            withAnimation(.none) {
              animationScale = 0.0
            }
          }
      }
    }
  }
  
  @State private var dragValueSize: CGSize = .zero
  
  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      Group {
        if let dragContext = dragContext, let dataPoint = dragContext.dataPoint {
          let offsetCenter = dragContext.location.x - (dragValueSize.width / 2.0)
          let offset = max(0, min(dragContext.size.width - dragValueSize.width, offsetCenter))
          Text(String(format: "%d", Int(dataPoint.value)))
            .padding(.horizontal, 8)
            .overlay(GeometryReader { proxy in
              Color.clear
                .preference(key: ValueSizePreferenceKey.self, value: proxy.size)
            })
            .frame(maxWidth: .infinity, alignment: .leading)
            .offset(x: offset)
        } else {
          Text("00:00")
            .hidden()
        }
      }
      .onPreferenceChange(ValueSizePreferenceKey.self) {
        self.dragValueSize = $0
      }
      .font(.footnote)
      fill
        .mask(
          LineChartShape(points: points)
            .padding(.vertical, 14)
            .drawingGroup() // Drawing group clips anything above it, so we need additional padding
            .overlay(
              finalDotView
                .padding(.vertical, 14) // But drag calculations need to use the correct coordinates without padding)
            )
        )
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, -14) // But drag calculations need to use the correct coordinates without padding
        .overlay(
          Group {
            if let dragContext = dragContext {
              Rectangle()
                .fill(Color(.secondaryButtonTint))
                .frame(width: 2)
                .frame(maxHeight: .infinity)
                .padding(.vertical, -10)
                .position(x: dragContext.location.x, y: dragContext.size.height / 2)
            }
          }
        )
        .overlay(GeometryReader { proxy in
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
        })
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
