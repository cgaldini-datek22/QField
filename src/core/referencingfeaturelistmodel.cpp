/***************************************************************************
  referencingfeaturelistmodel.cpp - ReferencingFeatureListModel

 ---------------------
 begin                : 1.3.2019
 copyright            : (C) 2019 by David Signer
 email                : david@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "referencingfeaturelistmodel.h"

ReferencingFeatureListModel::ReferencingFeatureListModel( QObject *parent )
  : QAbstractItemModel( parent )
{
}

QHash<int, QByteArray> ReferencingFeatureListModel::roleNames() const
{
  QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

  roles[DisplayString] = "displayString";
  roles[ReferencingFeature] = "referencingFeature";
  roles[NmReferencedFeature] = "nmReferencedFeature";
  roles[NmDisplayString] = "nmDisplayString";

  return roles;
}

QModelIndex ReferencingFeatureListModel::index( int row, int column, const QModelIndex &parent ) const
{
  Q_UNUSED( column )
  Q_UNUSED( parent )

  return createIndex( row, column, 1000 );
}

QModelIndex ReferencingFeatureListModel::parent( const QModelIndex &index ) const
{
  Q_UNUSED( index )

  return QModelIndex();
}

int ReferencingFeatureListModel::rowCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent )
  return mEntries.size();
}

int ReferencingFeatureListModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent )
  return 1;
}

QVariant ReferencingFeatureListModel::data( const QModelIndex &index, int role ) const
{
  if ( role == DisplayString )
    return mEntries.value( index.row() ).displayString;
  if ( role == ReferencingFeature )
    return mRelation.referencingLayer()->getFeature( mEntries.value( index.row() ).referencingFeatureId );
  if ( role == NmReferencedFeature )
    return mNmRelation.getReferencedFeature( mRelation.referencingLayer()->getFeature( mEntries.value( index.row() ).referencingFeatureId ) );
  if ( role == NmDisplayString )
    return nmDisplayString( mEntries.value( index.row() ).referencingFeatureId );
  return QVariant();
}

void ReferencingFeatureListModel::setFeature( const QgsFeature &feature )
{
  if ( mFeature == feature )
    return;

  mFeature = feature;
  reload();
}

QgsFeature ReferencingFeatureListModel::feature() const
{
  return mFeature;
}

void ReferencingFeatureListModel::setRelation( const QgsRelation &relation )
{
  mRelation = relation;
  reload();
}

QgsRelation ReferencingFeatureListModel::relation() const
{
  return mRelation;
}

void ReferencingFeatureListModel::setNmRelation( const QgsRelation &relation )
{
  mNmRelation = relation;
}

QgsRelation ReferencingFeatureListModel::nmRelation() const
{
  return mNmRelation;
}

void ReferencingFeatureListModel::setParentPrimariesAvailable( const bool parentPrimariesAvailable )
{
  mParentPrimariesAvailable = parentPrimariesAvailable;
}

bool ReferencingFeatureListModel::parentPrimariesAvailable() const
{
  return mParentPrimariesAvailable;
}

void ReferencingFeatureListModel::reload()
{
  if ( !mRelation.isValid() || !mFeature.isValid() )
    return;

  beginResetModel();

  mEntries.clear();

  if ( checkParentPrimaries() )
  {
    QgsFeatureIterator relatedFeaturesIt = mRelation.getRelatedFeatures( mFeature );
    QgsExpressionContext context = mRelation.referencingLayer()->createExpressionContext();
    QgsExpression expression( mRelation.referencingLayer()->displayExpression() );

    QgsFeature childFeature;
    while ( relatedFeaturesIt.nextFeature( childFeature ) )
    {
      context.setFeature( childFeature );
      mEntries.append( Entry( expression.evaluate( &context ).toString(), childFeature.id() ) );
    }
  }
  endResetModel();

  //set the property for parent primaries available status
  setParentPrimariesAvailable( checkParentPrimaries() );
}

void ReferencingFeatureListModel::deleteFeature( QgsFeatureId referencingFeatureId )
{
  mRelation.referencingLayer()->startEditing();
  mRelation.referencingLayer()->deleteFeature( referencingFeatureId );
  mRelation.referencingLayer()->commitChanges();
  reload();
}

bool ReferencingFeatureListModel::checkParentPrimaries()
{
  if ( !mRelation.isValid() || !mFeature.isValid() )
    return false;

  const auto fieldPairs = mRelation.fieldPairs();
  for ( QgsRelation::FieldPair fieldPair : fieldPairs )
  {
    if ( mFeature.attribute( fieldPair.second ).isNull() )
      return false;
  }
  return true;
}

QString ReferencingFeatureListModel::nmDisplayString( QgsFeatureId referencingFeatureId ) const
{
  QgsExpressionContext context = mNmRelation.referencedLayer()->createExpressionContext();
  QgsExpression expression( mNmRelation.referencedLayer()->displayExpression() );

  context.setFeature( mNmRelation.getReferencedFeature( mRelation.referencingLayer()->getFeature( referencingFeatureId ) ) );
  return expression.evaluate( &context ).toString();
}