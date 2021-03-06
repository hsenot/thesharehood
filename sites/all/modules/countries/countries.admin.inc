<?php
// $Id: countries.admin.inc,v 1.3 2010/09/18 03:58:36 aland Exp $

/**
 * @file
 * Admin page callbacks for the Countries module.
 */

/**
 * Menu callback; Displays a list of all countries.
 */
function countries_admin_overview() {
  $header = array();
  $header[] = array('data' => t('Name'), 'field' => 'c.name', 'sort' => 'asc');
  $header[] = array('data' => t('ISO2'), 'field' => 'c.iso2');
  $columns = variable_get('countries_admin_overview_columns', array('iso3' => t('ISO3'), 'numcode' => t('Number code'), 'continent' => t('Continent'), 'official_name' => t('Official name')));
  foreach ($columns as $key => $title) {
    $header[] = array('data' => $title, 'field' => 'c.' . $key);
  }
  $header[] = array('data' => t('Status'), 'field' => 'c.enabled');
  $header[] = array('data' => t('Operations'));
  $query = db_select('countries_country', 'c')->extend('PagerDefault')->extend('TableSort');

  $result = $query
    ->fields('c')
    ->orderByHeader($header)
    ->limit(50)
    ->execute();

  $destination = drupal_get_destination();
  $continents = countries_get_continents();

  include_once DRUPAL_ROOT . '/includes/iso.inc';
  $core_countries = _country_get_predefined_list();

  $rows = array();
  foreach ($result as $country) {
    $row = array();
    $row[] = l($country->name, 'admin/config/regional/countries/' . $country->iso2, array('query' => $destination));
    $row[] = $country->iso2;
    foreach ($columns as $key => $title) {
      switch ($key) {
        case 'continent':
          $row[] = isset($continents[$country->continent]) ? $continents[$country->continent] : t('Unknown');
          break;
        case 'numcode':
          $row[] = theme('countries_number', array('country' => $country));
          break;
        default:
          $row[] = check_plain($country->$key);
          break;
      }
    }
    $row[] = $country->enabled ? t('Enabled') : t('Disabled');
    $operations = l(t('edit'), 'admin/config/regional/countries/' . $country->iso2 , array('query' => $destination));
    if (!array_key_exists($country->iso2, $core_countries)) {
      $operations .= '&nbsp;&nbsp;&nbsp;' . l(t('delete'), 'admin/config/regional/countries/' . $country->iso2 . '/delete', array('query' => $destination));
    }
    $row[] = $operations;
    $rows[] = $row;
  }
  if (empty($rows)) {
    $rows[] = array(
      array('data' => t('No countries are available.'), 'colspan' => count($header)),
    );
  }

  $build['countries_table']  = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
  );
  $build['countries_pager'] = array('#theme' => 'pager');

  return $build;
}

/**
 * Menu callback; Display a country form.
 */
function countries_admin_page($country = NULL) {
  if (!isset($country->name)) {
    drupal_set_title(t('Add country'), PASS_THROUGH);
    $country = (object)array(
      'cid' => 0,
      'iso2' => '',
      'iso3' => '',
      'name' => '',
      'official_name' => '',
      'continent' => 'UN',
      'enabled' => 1,
      'numcode' => 0);

  }
  return drupal_get_form('countries_admin_form', $country);
}

/**
 * Generate a country form.
 *
 * @ingroup forms
 * @see countries_admin_form_validate()
 * @see countries_admin_form_submit()
 */
function countries_admin_form($form, &$form_state, $country) {

  $form = array();
  $form['cid'] = array(
    '#type' => 'value',
    '#value' => $country->cid,
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => $country->name,
    '#description' => t('Specify a unique name for this country.'),
    '#required' => TRUE,
    '#maxlength' => 95,
  );
  $iso2_editable = TRUE;
  if (!empty($country->iso2)) {
    include_once DRUPAL_ROOT . '/includes/iso.inc';
    $core_countries = _country_get_predefined_list();
    $iso2_editable = ! array_key_exists($country->iso2, $core_countries);
  }
  if ($iso2_editable) {
    $form['iso2'] = array(
      '#type' => 'textfield',
      '#title' => t('ISO2 code'),
      '#default_value' => $country->iso2,
      '#description' => t('Specify a unique ISO2 code for this country. This is used as the key to this country, changing it may result in the loss of data.'),
      '#required' => TRUE,
      '#maxlength' => 2,
    );
  }
  else {
    $form['iso2'] = array(
      '#type' => 'value',
      '#value' => $country->iso2,
    );
    $form['iso2_info'] = array(
      '#type' => 'textfield',
      '#title' => t('ISO2 code'),
      '#value' => $country->iso2,
      '#disabled' => TRUE,
      '#description' => t('Core country ISO2 codes are not editable.'),
    );
  }
  $form['iso3'] = array(
    '#type' => 'textfield',
    '#title' => t('ISO3 code'),
    '#default_value' => $country->iso3,
    '#description' => t('Specify a unique ISO3 code for this country.'),
    '#required' => FALSE,
    '#maxlength' => 3,
  );
  $form['official_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Official name'),
    '#default_value' => $country->official_name,
    '#description' => t('Specify the unique official name for this country.'),
    '#required' => FALSE,
    '#maxlength' => 127,
  );
  $form['numcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Number code'),
    '#default_value' => empty($country->numcode) ? '' : $country->numcode,
    '#description' => t('Specify a unique number code for this country.'),
    '#required' => FALSE,
    '#maxlength' => 5,
  );

  $form['continent'] = array('#type' => 'select',
    '#title' => t('Continent'),
    '#options' => countries_get_continents(),
    '#default_value' => $country->continent,
    '#required' => TRUE,
  );
  $form['enabled'] = array('#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#default_value' => $country->enabled,
  );

  if (!empty($country->iso2)) {
    $form['#country'] = $country;
  }

  $country = (object) $country;
  field_attach_form('country', $country, $form, $form_state);

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save'), '#weight' => 100);

  return $form;
}

/**
 * Validate country form submissions.
 */
function countries_admin_form_validate($form, &$form_state) {
  // Edits
  $unique_keys = array('name', 'iso2', 'iso3', 'official_name', 'numcode');

  foreach ($unique_keys as $key) {
    if (drupal_strlen($form_state['values'][$key])) {
      $iso2 = isset($form['#country']) ? $form['#country']->iso2 : FALSE;
      if ($key == 'iso2' || $key == 'iso3') {
        $form_state['values'][$key] = drupal_strtoupper($form_state['values'][$key]);
      }
      else {
        $form_state['values'][$key] = trim($form_state['values'][$key]);
      }
      switch($key) {
        case 'name':
          break;
        case 'iso2':
        case 'iso3':
          if (preg_match('/[^A-Z]/', $form_state['values'][$key])) {
            form_set_error($key, t('Please only use uppercase characters for ISO codes.'));
            continue;
          }
          else {
            $length = $key == 'iso2' ? 2 : 3;
            if (drupal_strlen($form_state['values'][$key]) != $length) {
              form_set_error($key, t('The ISO!length code must be exactly !length characters.', array('!length' => $length)));
              continue;
            }
          }
          break;
        case 'official_name':
          break;
        case 'numcode':
          if (preg_match('/[^0-9]/', $form_state['values'][$key])) {
            form_set_error($key, t('Only numbers are allowed for the number code.'));
            continue;
          }
          break;
      }
      if ($key == 'numcode' && empty($form_state['values'][$key])) {
        // Duplicate 0 values are OK.
      }
      elseif ((bool) db_query_range("SELECT 1 FROM {countries_country} WHERE iso2 <> :orginaliso2 AND LOWER({$key}) = LOWER(:$key)", 0, 1, array(':orginaliso2' => $iso2, ":$key" => $form_state['values'][$key]))->fetchField()) {
        form_set_error($key, t('The value %value is already taken.', array('%value' => $form_state['values'][$key])));
      }
    }
  }
}

/**
 * Process country form submissions.
 */
function countries_admin_form_submit($form, &$form_state) {
  $country = (object) $form_state['values'];
  $iso2 = isset($form['#country']) ? $form['#country']->iso2 : FALSE;

  field_attach_presave('country', $country);

  if ($iso2) {
    if ($country->iso2 != $iso2) {
      db_update('countries_country')
        ->fields(array('iso2' => $country->iso2))
        ->condition('iso2', $iso2)
        ->execute();
    }
    drupal_write_record('countries_country', $country, array('iso2'));
    field_attach_update('country', $country);
    $message = t('The country %country has been updated.', array('%country' => $country->name));
    field_cache_clear();
  }
  else {
    drupal_write_record('countries_country', $country);
    field_attach_insert('country', $country);
    $message = t('Added country %country.', array('%country' => $country->name));
  }
  // TODO: I18n
  // countries_locale_process($country);
  drupal_set_message($message);
  $form_state['redirect'] = 'admin/config/regional/countries';

}

/**
 * Menu callback; confirm deletion of a country.
 *
 * @ingroup forms
 * @see countries_admin_delete_submit()
 */
function countries_admin_delete($form, &$form_state, $country) {
  include_once DRUPAL_ROOT . '/includes/iso.inc';
  $core_countries = _country_get_predefined_list();
  if (array_key_exists($country->iso2, $core_countries)) {
    drupal_set_message(t('Core countries defined by the system can not be deleted.'), 'error');
    drupal_goto('admin/config/regional/countries');
  }

  $form['#country'] = $country;

  return confirm_form($form,
    t('Are you sure you want to delete the country %country?', array('%country' => $country->name)),
    'admin/config/regional/countries',
    t('References that use this country will become invalid. This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Process countries delete form submission.
 */
function countries_admin_delete_submit($form, &$form_state) {
  $country = $form['#country'];
  field_attach_delete('country', $country);
  db_delete('countries_country')
    ->condition('iso2', $country->iso2)
    ->execute();
  drupal_set_message(t('Deleted country %country.', array('%country' => $country->name)));
  $form_state['redirect'] = 'admin/config/content/countries';
}
