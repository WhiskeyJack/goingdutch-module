<?php

/**
 * @file
 *   Admin settings page.
 */

/**
 * Form builder; Create and display the Going Dutch configuration
 * settings form.
 */
function going_dutch_admin_form($form, &$form_state) {

  $form['gd_db_host'] = array(
      '#type' => 'textfield',
      '#title' => t('Going Dutch database host'),
      '#description' => t('Host address of the Going Dutch database.'),
      '#size' => 20,
      '#maxlength' => 40,
      '#required' => TRUE,
      '#default_value' => variable_get('gd_db_host', 'localhost'),
  );

  $form['gd_db_user'] = array(
      '#type' => 'textfield',
      '#title' => t('Going Dutch database user'),
      '#description' => t('Username to access the Going Dutch database.'),
      '#size' => 20,
      '#maxlength' => 40,
      '#required' => TRUE,
      '#default_value' => variable_get('gd_db_user', 'dutch'),
  );

  // check if a password has already been given previously. If not; require a new one
  $curpass = variable_get('gd_db_pass');
  $required = (empty($curpass)) ? TRUE : FALSE;
  $form['gd_db_pass'] = array(
      '#type' => 'password',
      '#title' => t('Going Dutch database password'),
      '#description' => t('Password to access the Going Dutch database.'),
      '#size' => 20,
      '#maxlength' => 40,
      '#required' => $required,
  );

  $form['gd_db_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Going Dutch database'),
      '#description' => t('Database name with Going Dutch tables.'),
      '#size' => 20,
      '#maxlength' => 40,
      '#required' => TRUE,
      '#default_value' => variable_get('gd_db_name', 'goingdutch'),
  );

  //$drivers = array('mysql' => 'MySQL', 'pgsql' => 'PostgreSQL', 'sqlite' => 'SQLite');
  $drivers = array('mysql' => 'MySQL');
  $form['gd_db_driver'] = array(
      '#type' => 'select',
      '#title' => t('Going Dutch database driver'),
      '#description' => t('Driver for Going Dutch database. Currently only MySQL has been tested.'),
      '#required' => TRUE,
      '#options' => $drivers,
      '#default_value' => variable_get('gd_db_driver', 'mysql'),
  );
  
  $form['gd_welcome_page'] = array(
      '#type' => 'textfield',
      '#title' => t('The welcome page (node) for anonymous users'),
      '#description' => t('The page with the Going Dutch welcome text. This page is shown when the user is not logged in.'),
      '#size' => 20,
      '#maxlength' => 40,
      '#required' => FALSE,
      '#default_value' => variable_get('gd_welcome_page', ''),
  );

  
  $form['gd_submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save settings'),
  );
  
  

  return $form;
}

/**
 * Save the configuration settings
 */
function going_dutch_admin_form_submit($form, &$form_state) {

  variable_set('gd_db_host', $form_state['values']['gd_db_host']);
  variable_set('gd_db_user', $form_state['values']['gd_db_user']);
  if (!empty($form_state['values']['gd_db_pass']))
    variable_set('gd_db_pass', $form_state['values']['gd_db_pass']);
  variable_set('gd_db_name', $form_state['values']['gd_db_name']);
  variable_set('gd_db_driver', $form_state['values']['gd_db_driver']);
  variable_set('gd_welcome_page', $form_state['values']['gd_welcome_page']);
  drupal_set_message(t('The settings have been saved'));
}