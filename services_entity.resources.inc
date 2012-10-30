<?php

function _services_entity_resource_access($op, $args) {
  $resourceclass = variable_get('services_entity_resource_class', 'ServicesEntityResourceController');
  $resource = new $resourceclass;
  return $resource->access($op, $args);
}

function _services_entity_resource_create($entity_type, $values) {
  $resourceclass = variable_get('services_entity_resource_class', 'ServicesEntityResourceController');
  $resource = new $resourceclass;
  return $resource->create($entity_type, $values);
}

function _services_entity_resource_retrieve($entity_type, $entity_key, $fields) {
  $resourceclass = variable_get('services_entity_resource_class', 'ServicesEntityResourceController');
  $resource = new $resourceclass;
  return $resource->retrieve($entity_type, $entity_key, $fields);
}

function _services_entity_resource_update($entity_type, $entity_id, $values) {
  $resourceclass = variable_get('services_entity_resource_class', 'ServicesEntityResourceController');
  $resource = new $resourceclass;
  return $resource->update($entity_type, $entity_id, $values);
}

function _services_entity_resource_delete($entity_type, $entity_id) {
  $resourceclass = variable_get('services_entity_resource_class', 'ServicesEntityResourceController');
  $resource = new $resourceclass;
  return $resource->delete($entity_type, $entity_id);
}

function _services_entity_resource_index($entity_type, $fields, $parameters, $page, $pagesize, $sort, $direction) {
  $resourceclass = variable_get('services_entity_resource_class', 'ServicesEntityResourceController');
  $resource = new $resourceclass;
  return $resource->index($entity_type, $fields, $parameters, $page, $pagesize, $sort, $direction);
}
